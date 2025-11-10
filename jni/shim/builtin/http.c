#include <string.h>
#include <dlfcn.h>
#include <android/log.h>

#define HTTP_IMPLEMENTATION
#define HTTP_LOG(...) __android_log_print(ANDROID_LOG_INFO, "knshim", __VA_ARGS__)
#include <netinet/in.h>
#include "../extern/http.h"

#include "lua_utils.h"
#include "../util.h"

#ifdef HTTP_ENABLE_MBEDTLS
struct {
	bool allow_without_cert;
	unsigned char *cert_data;
	size_t cert_data_size;
} gHttps;
#endif

typedef struct {
	http_t *context;
} knHttpContext;

enum {
	KN_HTTP_PENDING = 1,
	KN_HTTP_DONE,
	KN_HTTP_ERROR,
};

int knHttpRelease(lua_State *script);

int knHttpRequest_addmetatable(lua_State *script) {
	if (luaL_newmetatable(script, "knHttpContext")) {
		lua_pushstring(script, "__gc");
		lua_pushcfunction(script, knHttpRelease);
		lua_settable(script, -3);
	}
	
	lua_setmetatable(script, -2);
	
	return 0;
}

size_t seqenceLength(lua_State *L, int t) {
	/**
	 * Get the length of a table, even if its not array like
	 */
	
	lua_pushnil(L);
	
	size_t i;
	
	for (i = 0; lua_next(L, t); i++) {
		lua_pop(L, 1);
	}
	
	return i;
}

size_t fillHeaders(lua_State *L, int t, http_header_t *headers, size_t count) {
	// Push a copy of the table for reference purposes
	lua_pushvalue(L, t);
	
	// First key (dummy)
	lua_pushnil(L);
	
	// Iterate keys
	size_t i;
	
	for (i = 0; lua_next(L, -2) && i < count; i++) {
		// Push a temp copy of the key so we can safely tostring() it.
		lua_pushvalue(L, -2);
		
		// Copy to header table
		headers[i].name = lua_tostring(L, -1);
		headers[i].value = lua_tostring(L, -2);
		
		if (!headers[i].name || !headers[i].value) {
			luaL_error(L, "Invalid HTTP header name (%s) or value (%s): make sure keys and values of the headers table support being converted to a string using tostring().", headers[i].name ? headers[i].name : "<null>", headers[i].value ? headers[i].value : "<null>");
		}
		
		// Pop our temp key and the value
		lua_pop(L, 2);
	}
	
	// Pop the table copy
	lua_pop(L, 1);
	
	// Return number of headers copied (should be all of them)
	return i;
}

// HTTP
int knHttpRequest(lua_State *script) {
	/**
	 * request = knHttpRequest(method, url, [body, [headers]])
	 * 
	 * - method: "GET", "POST", "PUT", "DELETE", etc.
	 * - url: "http://" URL
	 * - body: none, nil or string representing request body
	 * - headers: none, nil or dictionary respresenting request headers
	 * 
	 * Creates and fires an HTTP request. The first argument should be the HTTP
	 * method, that is GET, POST, PUT, DELETE etc. The second is the URL to post
	 * to. The third argument is the body of the request. The fourth argument is
	 * an optional table of HTTP headers to include.
	 */
	
	if (lua_gettop(script) < 2) {
		luaL_error(script, "At least two arguments are required: method and url");
		return 0;
	}
	
	const char *method = lua_tostring(script, 1);
	
	if (!method) {
		luaL_error(script, "Method could not be converted to a string");
		return 0;
	}
	
	const char *url = lua_tostring(script, 2);
	
	if (!method) {
		luaL_error(script, "URL could not be converted to a string");
		return 0;
	}
	
#ifdef HTTP_ENABLE_MBEDTLS
	if (!memcmp("https://", url, 8) && !gHttps.allow_without_cert && !gHttps.cert_data) {
		luaL_error(script, "Certificate verification set to required but no HTTPS certificate has been installed");
		return 0;
	}
#endif
	
	size_t body_size = 0;
	const char *body = lua_tolstring(script, 3, &body_size);
	
	size_t num_headers = lua_istable(script, 4) ? seqenceLength(script, 4) : 0;
	http_header_t headers[num_headers];
	
	if (num_headers) {
		fillHeaders(script, 4, headers, num_headers);
	}
	
#ifdef HTTP_ENABLE_MBEDTLS
	http_t *request = http_request(method, url, body, body_size, num_headers ? headers : NULL, num_headers, gHttps.cert_data, gHttps.cert_data_size, NULL);
#else
	http_t *request = http_request(method, url, body, body_size, num_headers ? headers : NULL, num_headers, NULL);
#endif
	
	if (!request) {
		luaL_error(script, "Could not create request object");
		return 0;
	}
	
	knHttpContext *ctx = lua_newuserdata(script, sizeof *ctx);
	ctx->context = request;
	
	knHttpRequest_addmetatable(script);
	
	return 1;
}

int knHttpUpdate(lua_State *script) {
	/**
	 * Process the HTTP request. If still processing, return nil. If the request
	 * errored, return KN_HTTP_ERROR. If the request succeeded, return KN_HTTP_DONE.
	 */
	
	if (lua_gettop(script) < 1) {
		lua_pushnil(script);
		return 1;
	}
	
	knHttpContext *ctx = lua_touserdata(script, 1);
	
	if (!ctx || !ctx->context) {
		lua_pushnil(script);
		return 1;
	}
	
	http_status_t status = http_process(ctx->context);
	
	switch (status) {
		case HTTP_STATUS_PENDING:
			lua_pushinteger(script, KN_HTTP_PENDING);
			break;
		case HTTP_STATUS_FAILED:
			lua_pushinteger(script, KN_HTTP_ERROR);
			break;
		case HTTP_STATUS_COMPLETED:
			lua_pushinteger(script, KN_HTTP_DONE);
			break;
		default:
			lua_pushinteger(script, KN_HTTP_ERROR);
			break;
	}
	
	return 1;
}

int knHttpData(lua_State *script) {
	/**
	 * Return the data if succeeded and still allocated, otherwise return nil.
	 */
	
	if (lua_gettop(script) < 1) {
		lua_pushnil(script);
		return 1;
	}
	
	knHttpContext *ctx = lua_touserdata(script, 1);
	
	if (!ctx || !ctx->context) {
		lua_pushnil(script);
		return 1;
	}
	
	if (ctx->context->status == HTTP_STATUS_COMPLETED || ctx->context->status == HTTP_STATUS_PENDING) {
		lua_pushlstring(script, ctx->context->response_data, ctx->context->response_size);
	}
	else {
		lua_pushnil(script);
	}
	
	return 1;
}

int knHttpSave(lua_State *script) {
	/**
	 * knHttpSave(request, path)
	 * 
	 * Save the data to a given file.
	 */
	
	if (lua_gettop(script) < 2) {
		luaL_error(script, "Not enough arguments to knHttpSave");
		return 0;
	}
	
	knHttpContext *ctx = lua_touserdata(script, 1);
	
	if (!ctx || !ctx->context) {
		luaL_error(script, "Context is nil");
		return 0;
	}
	
	const char *path = lua_tostring(script, 2);
	
	if (!path) {
		luaL_error(script, "Path is not valid");
		return 0;
	}
	
	FILE *file = fopen(path, "wb");
	
	if (!file) {
		luaL_error(script, "Failed to open file write stream");
		return 0;
	}
	
	size_t written = fwrite(ctx->context->response_data, 1, ctx->context->response_size, file);
	
	fclose(file);
	
	if (written != ctx->context->response_size) {
		remove(path);
		luaL_error(script, "File was not completely written");
		return 0;
	}
	
	return 0;
}

int knHttpDataSize(lua_State *script) {
	/**
	 * Return the size of the data or 0 if there is none.
	 */
	
	if (lua_gettop(script) < 1) {
		lua_pushnil(script);
		return 1;
	}
	
	knHttpContext *ctx = lua_touserdata(script, 1);
	
	if (!ctx || !ctx->context) {
		lua_pushinteger(script, 0);
		return 1;
	}
	
	lua_pushinteger(script, ctx->context->response_size);
	
	return 1;
}

int knHttpGetHeader(lua_State *script) {
	/**
	 * (string|nil) header = knHttpGetHeader(request, (string) name, (int) nth)
	 * 
	 * Return a string representing the content type of the data
	 */
	
	if (lua_gettop(script) < 1) {
		lua_pushnil(script);
		return 1;
	}
	
	knHttpContext *ctx = lua_touserdata(script, 1);
	
	if (!ctx || !ctx->context) {
		lua_pushnil(script);
		return 1;
	}
	
	const char *name = lua_tostring(script, 2);
	
	if (!name) {
		lua_pushnil(script);
		return 1;
	}
	
	size_t nth = lua_tointeger(script, 3);
	
	const char *value = http_get_header(ctx->context, name, nth);
	
	if (value) {
		lua_pushstring(script, value);
	}
	else {
		lua_pushnil(script);
	}
	
	return 1;
}

int knHttpError(lua_State *script) {
	/**
	 * Return a string describing the HTTP error.
	 * Note that the string may be empty even if there is an error, such as when
	 * there are connection issues.
	 */
	
	if (lua_gettop(script) < 1) {
		lua_pushnil(script);
		return 1;
	}
	
	knHttpContext *ctx = lua_touserdata(script, 1);
	
	if (!ctx || !ctx->context) {
		lua_pushnil(script);
		return 1;
	}
	
	if (ctx->context->reason_phrase) {
		lua_pushstring(script, ctx->context->reason_phrase);
	}
	else {
		lua_pushnil(script);
	}
	
	return 1;
}

int knHttpErrorCode(lua_State *script) {
	/**
	 * Return the integer HTTP error code. Note that an error code of zero does not
	 * mean there is not an error, for example in the condition of connection
	 * issues.
	 */
	
	if (lua_gettop(script) < 1) {
		lua_pushnil(script);
		return 1;
	}
	
	knHttpContext *ctx = lua_touserdata(script, 1);
	
	if (!ctx || !ctx->context) {
		lua_pushnil(script);
		return 1;
	}
	
	lua_pushinteger(script, ctx->context->status_code);
	
	return 1;
}

int knHttpRelease(lua_State *script) {
	/**
	 * Release the http request context.
	 */
	
	if (lua_gettop(script) < 1) {
		return 0;
	}
	
	knHttpContext *ctx = lua_touserdata(script, 1);
	
	if (ctx) {
		if (ctx->context) {
			http_release(ctx->context);
		}
		
		ctx->context = NULL;
	}
	
	return 0;
}

void *libNXArchive;
const char *(*NXExtractArchiveFromBuffer)(const char *location, size_t size, const void *buf);

int knHttpExtractNxArchive(lua_State *script) {
	/**
	 * errorMsg = knHttpExtractNxArchive(httpRequest, extractDir)
	 * 
	 * Extracts an NXArchive from the HTTP response data. Returns either false
	 * on success or a string explaining the error on failure.
	 */
	
	if (!libNXArchive) {
		libNXArchive = dlopen("libNXArchive.so", RTLD_NOW | RTLD_GLOBAL);
		
		if (!libNXArchive) {
			lua_pushstring(script, "nxarchive library not present");
			return 1;
		}
		
		NXExtractArchiveFromBuffer = dlsym(libNXArchive, "NXExtractArchiveFromBuffer");
	}
	
	if (!NXExtractArchiveFromBuffer) {
		lua_pushstring(script, "symbol 'NXExtractArchiveFromBuffer' not found");
		return 1;
	}
	
	if (lua_gettop(script) < 2) {
		lua_pushstring(script, "not enough params");
		return 1;
	}
	
	knHttpContext *ctx = lua_touserdata(script, 1);
	
	if (!ctx || !ctx->context) {
		lua_pushstring(script, "http context is null");
		return 1;
	}
	
	const char *dest = lua_tostring(script, 2);
	
	if (!dest) {
		lua_pushstring(script, "dest is null");
		return 1;
	}
	
	const char *err = NXExtractArchiveFromBuffer(dest, ctx->context->response_size, ctx->context->response_data);
	
	if (err) {
		lua_pushstring(script, err);
	}
	else {
		lua_pushboolean(script, 0);
	}
	
	return 1;
}

#ifdef HTTP_ENABLE_MBEDTLS
int knHttpsCert(lua_State *L) {
	if (lua_gettop(L) == 0) {
		free(gHttps.cert_data);
		gHttps.cert_data = NULL;
		gHttps.cert_data_size = 0;
	}
	
	size_t cert_size;
	const char *cert = lua_tolstring(L, 1, &cert_size);
	
	if (!cert) {
		luaL_error(L, "Certifiate data is a nil value or not convertable to a string; if you loaded from an asset, maybe that asset doesn't exist?");
	}
	
	unsigned char *new_cert_buf = malloc(cert_size);
	
	if (!new_cert_buf) {
		luaL_error(L, "Could not allocate new cert buffer!");
		return 0;
	}
	
	memcpy(new_cert_buf, cert, cert_size);
	free(gHttps.cert_data);
	gHttps.cert_data = new_cert_buf;
	gHttps.cert_data_size = cert_size;
	
	return 0;
}

int knHttpsNoCert(lua_State *L) {
	const char *magic = lua_tostring(L, 1);
	
	if (magic && !strcmp(magic, "The foxes whispher in your ear: \"Here lies dangerous code!\"")) {
		gHttps.allow_without_cert = true;
	}
	
	return 0;
}
#endif

int knEnableHttp(lua_State *script) {
	lua_register(script, "knHttpRequest", knHttpRequest);
	lua_register(script, "knHttpUpdate", knHttpUpdate);
	lua_register(script, "knHttpData", knHttpData);
	lua_register(script, "knHttpDataSize", knHttpDataSize);
	lua_register(script, "knHttpSave", knHttpSave);
	lua_register(script, "knHttpGetHeader", knHttpGetHeader);
	lua_register(script, "knHttpError", knHttpError);
	lua_register(script, "knHttpErrorCode", knHttpErrorCode);
	lua_register(script, "knHttpRelease", knHttpRelease);
	lua_register(script, "knHttpExtractNxArchive", knHttpExtractNxArchive);
#ifdef HTTP_ENABLE_MBEDTLS
	lua_register(script, "knHttpsCert", knHttpsCert);
	lua_register(script, "knHttpsNoCert", knHttpsNoCert);
#endif
	lua_pushinteger(script, KN_HTTP_PENDING); lua_setglobal(script, "KN_HTTP_PENDING");
	lua_pushinteger(script, KN_HTTP_DONE); lua_setglobal(script, "KN_HTTP_DONE");
	lua_pushinteger(script, KN_HTTP_ERROR); lua_setglobal(script, "KN_HTTP_ERROR");
	
	return 0;
}
