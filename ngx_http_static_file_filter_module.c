#include <nginx.h>
#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>

static ngx_int_t ngx_http_static_file_filter_handler(ngx_http_request_t *r);
static ngx_int_t ngx_http_static_file_filter_init(ngx_conf_t *cf);
static char * ngx_http_static_file_filter_conf(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
static void * ngx_http_static_file_filter_create_loc_conf(ngx_conf_t *cf);
static void * ngx_http_static_file_filter_merge_loc_conf(ngx_conf_t *cf, void *parent, void *child);

typedef struct {
	ngx_array_t *types;
} ngx_http_static_file_filter_loc_conf_t;

static ngx_command_t ngx_http_static_file_filter_commands[] = {
	{ ngx_string("static_file_filter"),
	  NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_1MORE,
	  ngx_http_static_file_filter_conf,
	  NGX_HTTP_LOC_CONF_OFFSET,
	  0,
	  NULL },

	ngx_null_command
};

ngx_http_module_t  ngx_http_static_file_filter_module_ctx = {
    NULL,
    ngx_http_static_file_filter_init,

    NULL,
    NULL,

    NULL,
    NULL,

    ngx_http_static_file_filter_create_loc_conf,
    ngx_http_static_file_filter_merge_loc_conf
};

ngx_module_t  ngx_http_static_file_filter_module = {
    NGX_MODULE_V1,
    &ngx_http_static_file_filter_module_ctx,
    ngx_http_static_file_filter_commands,
    NGX_HTTP_MODULE,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NGX_MODULE_V1_PADDING
};

static ngx_int_t
ngx_http_static_file_filter_handler(ngx_http_request_t *r)
{
	ngx_http_static_file_filter_loc_conf_t *sffcf;
	ngx_str_t *value;
	ngx_uint_t i, n, len;

	sffcf = ngx_http_get_module_loc_conf(r, ngx_http_static_file_filter_module);
	if (sffcf == NULL || sffcf->types == NULL) {
		return NGX_DECLINED;
	}

	n = sffcf->types->nelts;
	value = sffcf->types->elts;

	for (i = 0; i < n; ++i) {
		len = value[i].len;
		if ((r->uri.len > len+1) &&
		    (r->uri.data[r->uri.len-len-1] == '.') &&
		    (ngx_strncasecmp(&r->uri.data[r->uri.len-len],
				     value[i].data,
				     len) == 0)) {
			return NGX_HTTP_FORBIDDEN;
		}
	}

	return NGX_DECLINED;
}

void
append_type(ngx_array_t *types, ngx_str_t *t)
{
	ngx_str_t *type;

	type = ngx_array_push(types);
	if (type == NULL) {
		return;
	}

	type->len = t->len;
	type->data = ngx_pstrdup(types->pool, t);
	return;
}

static char *
ngx_http_static_file_filter_conf(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
	ngx_http_static_file_filter_loc_conf_t *sffcf;
	ngx_uint_t n;
	ngx_str_t *value;
	ngx_str_t *type;
	ngx_uint_t i;

	sffcf = conf;
	n = cf->args->nelts;
  	value = cf->args->elts;
	
	if (sffcf->types == NULL) {
		sffcf->types = ngx_array_create(cf->pool,
						n,
						sizeof(ngx_str_t));
		if (sffcf->types == NULL) {
			return NGX_CONF_ERROR;
		}
	}

	for (i = 0; i < n; ++i) {
		append_type(sffcf->types, &value[i]);
	}

	return NGX_CONF_OK;
}

static void *
ngx_http_static_file_filter_create_loc_conf(ngx_conf_t *cf)
{
	ngx_http_static_file_filter_loc_conf_t *conf;

	conf = ngx_pcalloc(cf->pool, sizeof(ngx_http_static_file_filter_loc_conf_t));
	if (conf == NULL) {
		return NULL;
	}

	conf->types = NULL;

	return conf;
}

static void *
ngx_http_static_file_filter_merge_loc_conf(ngx_conf_t *cf, void *parent, void *child)
{
	ngx_http_static_file_filter_loc_conf_t *prev = parent;
	ngx_http_static_file_filter_loc_conf_t *conf = child;
	ngx_uint_t i;
	ngx_str_t *value;

	if (conf->types == NULL) {
		conf->types = prev->types;
	}
	else if (prev->types != NULL) {
		value = prev->types->elts;

		for (i = 0; i < prev->types->nelts; ++i) {
			append_type(conf->types, &value[i]);
		}
	}

	return NGX_CONF_OK;
}

static ngx_int_t
ngx_http_static_file_filter_init(ngx_conf_t *cf)
{
	ngx_http_handler_pt        *h;
	ngx_http_core_main_conf_t  *cmcf;

	cmcf = ngx_http_conf_get_module_main_conf(cf, ngx_http_core_module);

	h = ngx_array_push(&cmcf->phases[NGX_HTTP_CONTENT_PHASE].handlers);
	if (h == NULL) {
		return NGX_ERROR;
	}

	*h = ngx_http_static_file_filter_handler;

	return NGX_OK;
}
