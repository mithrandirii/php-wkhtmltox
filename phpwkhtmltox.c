#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>

#include "php.h"
#include "phpwkhtmltox.h"
#include "wkhtmltox/pdf.h"
#include "wkhtmltox/image.h"

ZEND_DECLARE_MODULE_GLOBALS(phpwkhtmltox)

zval *wkhtmltox_errorlog;

static zend_function_entry phpwkhtmltox_functions[] = {
    PHP_FE(wkhtmltox_convert, NULL)
    {NULL, NULL, NULL}
};

static void php_phpwkhtmltox_init_globals(zend_phpwkhtmltox_globals *phpwkhtmltox_globals)
{
    phpwkhtmltox_globals->wkhtmltoimage_initialized = 0;
    phpwkhtmltox_globals->wkhtmltopdf_initialized = 0;
}

PHP_MINIT_FUNCTION(phpwkhtmltox)
{
    ZEND_INIT_MODULE_GLOBALS(phpwkhtmltox, php_phpwkhtmltox_init_globals, NULL);
    
    return SUCCESS;
}

PHP_MSHUTDOWN_FUNCTION(phpwkhtmltox)
{
    if (PHPWKHTMLTOX_G(wkhtmltoimage_initialized)) {
        wkhtmltoimage_deinit();
    }
    
    if (PHPWKHTMLTOX_G(wkhtmltopdf_initialized)) {
        wkhtmltopdf_deinit();
    }
    
    return SUCCESS;
}

PHP_RINIT_FUNCTION(phpwkhtmltox)
{
    return SUCCESS;
}

PHP_RSHUTDOWN_FUNCTION(phpwkhtmltox)
{
    return SUCCESS;
}

PHP_MINFO_FUNCTION(phpwkhtmltox)
{
    php_info_print_table_start();
    php_info_print_table_row(2, "phpwkhtmltox", "enabled");
    php_info_print_table_row(2, "php extension version", PHP_PHPWKHTMLTOX_VERSION);
    php_info_print_table_row(2, "libwkhtmltox version", wkhtmltopdf_version());
    php_info_print_table_row(2, "extended wkhtmltox-qt support", (wkhtmltopdf_extended_qt() == 1) ? "yes" : "no");
    php_info_print_table_end();
}

zend_module_entry phpwkhtmltox_module_entry = {
#if ZEND_MODULE_API_NO >= 20010901
    STANDARD_MODULE_HEADER,
#endif
    PHP_PHPWKHTMLTOX_EXTNAME,
    phpwkhtmltox_functions,
    PHP_MINIT(phpwkhtmltox),
    PHP_MSHUTDOWN(phpwkhtmltox),
    PHP_RINIT(phpwkhtmltox),
    PHP_RSHUTDOWN(phpwkhtmltox),
    PHP_MINFO(phpwkhtmltox),
#if ZEND_MODULE_API_NO >= 20010901
    PHP_PHPWKHTMLTOX_VERSION,
#endif
    STANDARD_MODULE_PROPERTIES
};

#ifdef COMPILE_DL_PHPWKHTMLTOX
ZEND_GET_MODULE(phpwkhtmltox)
#endif

/**
 * bool wkhtmltox_convert(format, array global_settings, 2d array settings)
 */
PHP_FUNCTION(wkhtmltox_convert)
{
    // used for iterating over the object params
    zval **data;
    HashTable *params_hash;
    HashPosition pointer;
    int params_count;
    
    // temporarily holds return value
    int ret;
    
    // receives the format parameter
    char *format;
    int format_len;
    
    // receives the global and object parameters
    zval *global_params;
    zval *object_params;

    // receives the errorlog array reference parameter
    zval *errorlog;

    // input buffer
    zval *input;

    // output buffer
    const unsigned char *output;
    long output_len;

    // object check
    int object_added = 0;

    // parse out parameters passed
    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "saa|z", 
            &format, &format_len, &global_params, &object_params, &errorlog) == FAILURE) {
        RETURN_NULL();
    }

    if (strcmp(format, "image") == 0) {
        if (!PHPWKHTMLTOX_G(wkhtmltoimage_initialized)) {
            PHPWKHTMLTOX_G(wkhtmltoimage_initialized) = wkhtmltoimage_init(0);
        }
        
        wkhtmltoimage_global_settings *global_settings = wkhtmltoimage_create_global_settings();
        
        wkhtmltox_set_params((void *)global_settings, (fp)wkhtmltoimage_set_global_setting, global_params, input);
        
        wkhtmltoimage_converter *c = wkhtmltoimage_create_converter(global_settings, NULL);
        ret = wkhtmltoimage_convert(c);
        wkhtmltoimage_destroy_converter(c);
    } else if (strcmp(format, "pdf") == 0) {
        if (!PHPWKHTMLTOX_G(wkhtmltopdf_initialized)) {
            PHPWKHTMLTOX_G(wkhtmltopdf_initialized) = wkhtmltopdf_init(0);
        }
        
        wkhtmltopdf_global_settings *global_settings = wkhtmltopdf_create_global_settings();
        wkhtmltox_set_params((void *)global_settings, (fp)wkhtmltopdf_set_global_setting, global_params, input);
        
        wkhtmltopdf_converter *c = wkhtmltopdf_create_converter(global_settings);

	MAKE_STD_ZVAL(wkhtmltox_errorlog);
	array_init(wkhtmltox_errorlog);
	wkhtmltopdf_set_warning_callback(c, (wkhtmltopdf_str_callback)wkhtmltox_warning);
	wkhtmltopdf_set_error_callback(c, (wkhtmltopdf_str_callback)wkhtmltox_error);
        
        params_hash = Z_ARRVAL_P(object_params);
        params_count = zend_hash_num_elements(params_hash);
        for(zend_hash_internal_pointer_reset_ex(params_hash, &pointer); 
                zend_hash_get_current_data_ex(params_hash, (void **)&data, &pointer) == SUCCESS; 
                zend_hash_move_forward_ex(params_hash, &pointer)) {
            zval temp = **data;
            zval_copy_ctor(&temp);
            
            if (Z_TYPE(temp) == IS_ARRAY) {
                wkhtmltopdf_object_settings *object_settings = wkhtmltopdf_create_object_settings();
    		MAKE_STD_ZVAL(input);
                switch (wkhtmltox_set_params((void *)object_settings, (fp)wkhtmltopdf_set_object_setting, &temp, input)) {
                	case 0:
				wkhtmltopdf_add_object(c, object_settings, NULL);
				object_added = 1;
				break;
			case 1:
				convert_to_string(input);
				wkhtmltopdf_add_object(c, object_settings, Z_STRVAL_P(input));
				object_added = 1;
				break;
			default:
				wkhtmltox_error(c, "an object has no/empty page and no/empty html attribute. this means no input data can be found !"); 
		}
            }
            
            zval_dtor(&temp);
        }
        
        if (object_added == 1) {
		ret = wkhtmltopdf_convert(c);
		output_len = wkhtmltopdf_get_output(c, &output);
		if (output_len > 1) {
    			RETVAL_STRINGL(output, output_len, 1);
		}
       		wkhtmltopdf_destroy_converter(c);
        	zval_dtor(input);
	}
	
	ZVAL_ZVAL(errorlog, wkhtmltox_errorlog, 1, 1);
    }
    
    if (output_len < 1) {
	RETVAL_BOOL(ret);
    }
}

int wkhtmltox_set_params(void *settings, fp set_function, zval *params, zval *input)
{
    zval **data;
    HashTable *params_hash;
    HashPosition pointer;
    int params_count;
    
    char *key;
    int key_len;
    long index;

    int page = 0;
    int html = 0;
    
    params_hash = Z_ARRVAL_P(params);
    params_count = zend_hash_num_elements(params_hash);
    for(zend_hash_internal_pointer_reset_ex(params_hash, &pointer); 
            zend_hash_get_current_data_ex(params_hash, (void **)&data, &pointer) == SUCCESS; 
            zend_hash_move_forward_ex(params_hash, &pointer)) {
        zval temp = **data;
        zval_copy_ctor(&temp);
        
        if (zend_hash_get_current_key_ex(params_hash, &key, &key_len, &index, 0, &pointer) == HASH_KEY_IS_STRING) {
            switch (Z_TYPE(temp)) {
                case IS_BOOL:
                    set_function(settings, key, Z_BVAL(temp) ? "true" : "false");
                    break;
                default:
                    convert_to_string(&temp);
                case IS_STRING:
		    if (strcmp(key, "page") == 0 || strcmp(key, "out") == 0) {
			//TODO do checking of page/out url/file according to php security settings like open_basedir, allow_url_fopen a.s.o
			// php main fopenwrapper ?! php_check_specific_open_basedir php_check_open_basedir
			//php_printf("key: %s, val: %s\n", key, Z_STRVAL(temp));
		    }
		    if (strcmp(key, "page") == 0 && Z_STRLEN(temp) > 0) {
			page = 1;
		    }
		    if (strcmp(key, "html") == 0 && Z_STRLEN(temp) > 0) {
			ZVAL_STRING(input, Z_STRVAL(temp), 1);
			html = 1;
		    }
                    set_function(settings, key, Z_STRVAL(temp));
                    break;
            }
        }
        
        zval_dtor(&temp);
    }

    if (page == 1) {
	return 0;
    } else if (page == 0 && html == 1) {
	return 1;
    } else {
	return 2;
    }
}

void wkhtmltox_warning(wkhtmltopdf_converter *c, const char *msg) {
	zval *warning;
    	MAKE_STD_ZVAL(warning);
	array_init(warning);
	add_assoc_string(warning, "status", "warning", 1);
	add_assoc_string(warning, "message", (char *)msg, 1);
	add_next_index_zval(wkhtmltox_errorlog, warning);
}

void wkhtmltox_error(wkhtmltopdf_converter *c, const char *msg) {
	zval *error;
    	MAKE_STD_ZVAL(error);
	array_init(error);
	add_assoc_string(error, "status", "error", 1);
	add_assoc_string(error, "message", (char *)msg, 1);
	add_next_index_zval(wkhtmltox_errorlog, error);
}
