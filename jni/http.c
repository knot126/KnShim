#define HTTP_IMPLEMENTATION
#include <netinet/in.h>
#include "extern/http.h"

#include <string.h>
#include <android/log.h>

#include "lua/lua.h"
#include "lua/lualib.h"
#include "lua/lauxlib.h"

#include "util.h"

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
	 * getRequest = knHttpRequest(url)
	 * postRequest = knHttpRequest(url, body)
	 * request = knHttpRequest(method, url, body, [headers])
	 * 
	 * (For legacy GET and POST requests:)
	 * Create an HTTP GET or POST request. The first argument should be a
	 * url string. The second argument is an optional POST body. If a body
	 * is not specified GET is used instead of POST.
	 * 
	 * (For the modern API:)
	 * Creates an HTTP request. The first argument should be the HTTP method,
	 * that is GET, POST, PUT, DELETE etc. The second is the URL to post to.
	 * The third argument is the body of the request. If a body is not needed
	 * or the request type does not use a body, an empty string can be used to
	 * exclude the request body. The fourth argument is an optional table of
	 * HTTP headers to include.
	 */
	
	if (lua_gettop(script) < 1) {
		lua_pushnil(script);
		return 1;
	}
	
	const char *url = lua_tostring(script, 1);
	
	if (!url) {
		lua_pushnil(script);
		return 1;
	}
	
	int top = lua_gettop(script);
	
	http_t *request;
	
	if (top == 1) {
		__android_log_print(ANDROID_LOG_WARN, "smashhit", "Legacy http API (GET)");
		request = http_request("GET", url, NULL, 0, NULL, 0, NULL);
	}
	else if (top == 2) {
		__android_log_print(ANDROID_LOG_WARN, "smashhit", "Legacy http API (POST)");
		size_t size = 0;
		const char *body = lua_tolstring(script, 2, &size);
		
		request = http_request("POST", url, body, size, NULL, 0, NULL);
	}
	else {
		const char *method = lua_tostring(script, 1);
		url = lua_tostring(script, 2);
		
		size_t size = 0;
		const char *body = lua_tolstring(script, 3, &size);
		
		// Handle headers (or dont)
		if (lua_istable(script, 4)) {
			size_t num_headers = seqenceLength(script, 4);
			http_header_t headers[num_headers];
			
			__android_log_print(ANDROID_LOG_INFO, "smashhit", "Write %zu http headers", num_headers);
			
			fillHeaders(script, 4, headers, num_headers);
			
			request = http_request(method, url, body, size, headers, num_headers, NULL);
		}
		else {
			__android_log_print(ANDROID_LOG_INFO, "smashhit", "No headers");
			
			request = http_request(method, url, body, size, NULL, 0, NULL);
		}
	}
	
	if (!request) {
		lua_pushnil(script);
		return 1;
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
	
	if (ctx->context->status == HTTP_STATUS_COMPLETED) {
		lua_pushlstring(script, ctx->context->response_data, ctx->context->response_size);
	}
	else {
		lua_pushnil(script);
	}
	
	return 1;
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
// END HTTP

// START HTTP EXTRAS
const char *NXExtractArchiveFromBuffer(const char *location, size_t size, const void *buf);

int knHttpExtractNxArchive(lua_State *script) {
	/**
	 * errorMsg = knHttpExtractNxArchive(httpRequest, extractDir)
	 * 
	 * Extracts an NXArchive from the HTTP response data. Returns either false
	 * on success or a string explaining the error on failure.
	 */
	
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
// END HTTP EXTRAS

int knEnableHttp(lua_State *script) {
	lua_register(script, "knHttpRequest", knHttpRequest);
	lua_register(script, "knHttpUpdate", knHttpUpdate);
	lua_register(script, "knHttpData", knHttpData);
	lua_register(script, "knHttpDataSize", knHttpDataSize);
	lua_register(script, "knHttpGetHeader", knHttpGetHeader);
	lua_register(script, "knHttpError", knHttpError);
	lua_register(script, "knHttpErrorCode", knHttpErrorCode);
	lua_register(script, "knHttpRelease", knHttpRelease);
	lua_register(script, "knHttpExtractNxArchive", knHttpExtractNxArchive);
	lua_pushinteger(script, KN_HTTP_PENDING); lua_setglobal(script, "KN_HTTP_PENDING");
	lua_pushinteger(script, KN_HTTP_DONE); lua_setglobal(script, "KN_HTTP_DONE");
	lua_pushinteger(script, KN_HTTP_ERROR); lua_setglobal(script, "KN_HTTP_ERROR");
	
	return 0;
}
