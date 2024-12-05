#define HTTP_IMPLEMENTATION
#include <netinet/in.h>
#include "extern/http.h"

#include <string.h>
#include <android/log.h>

#include "lua/lua.h"
#include "lua/lualib.h"
#include "lua/lauxlib.h"

typedef struct {
    http_t *context;
} knHttpContext;

enum {
    KN_HTTP_PENDING = 1,
    KN_HTTP_DONE,
    KN_HTTP_ERROR,
};

// HTTP
int knHttpRequest(lua_State *script) {
    /**
     * Create an HTTP GET or POST request. The first argument should be a
     * url string. The second argument is an optional POST body. If a body
     * is not specified GET is used instead of POST.
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
    
    http_t *request;
    
    if (lua_gettop(script) == 1) {
        request = http_get(url, NULL);
    }
    else {
        size_t size = 0;
        const char *body = lua_tolstring(script, 2, &size);
        
        request = http_post(url, body, size, NULL);
    }
    
    if (!request) {
        lua_pushnil(script);
        return 1;
    }
    
    knHttpContext *ctx = lua_newuserdata(script, sizeof *ctx);
    ctx->context = request;
    
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

int knHttpContentType(lua_State *script) {
    /**
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
    
    if (ctx->context->content_type && strlen(ctx->context->content_type) != 0) {
        lua_pushstring(script, ctx->context->content_type);
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
        http_release(ctx->context);
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
    lua_register(script, "knHttpContentType", knHttpContentType);
    lua_register(script, "knHttpError", knHttpError);
    lua_register(script, "knHttpErrorCode", knHttpErrorCode);
    lua_register(script, "knHttpRelease", knHttpRelease);
    lua_register(script, "knHttpExtractNxArchive", knHttpExtractNxArchive);
    lua_pushinteger(script, KN_HTTP_PENDING); lua_setglobal(script, "KN_HTTP_PENDING");
    lua_pushinteger(script, KN_HTTP_DONE); lua_setglobal(script, "KN_HTTP_DONE");
    lua_pushinteger(script, KN_HTTP_ERROR); lua_setglobal(script, "KN_HTTP_ERROR");

    return 0;
}
