/*
  +----------------------------------------------------------------------+
  | PHP Version 5                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 1997-2010 The PHP Group                                |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.01 of the PHP license,      |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_01.txt                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Author: Alexey Zakhlestin <indeyets@php.net>                         |
  +----------------------------------------------------------------------+
*/

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <php.h>
#include <zend_interfaces.h>

#include "php_gobject.h"

static zend_bool zhashtable_to_gvalue(HashTable *zhash, GValue *gvalue)
{
	GValueArray *array = g_value_array_new(zend_hash_num_elements(zhash));

	HashPosition iterator;
	zval **tmp_zvalue;

	zend_hash_internal_pointer_reset_ex(zhash, &iterator);
	while (zend_hash_get_current_data_ex(zhash, (void **)&tmp_zvalue, &iterator) == SUCCESS) {
		GValue tmp_gvalue = {0,};

		if (FALSE == zval_to_gvalue(*tmp_zvalue, &tmp_gvalue)) {
			g_value_array_free(array);
			return FALSE;
		}

		g_value_array_append(array, &tmp_gvalue);
		g_value_unset(&tmp_gvalue);

		zend_hash_move_forward_ex(zhash, &iterator);
	}

	g_value_init(gvalue, G_TYPE_VALUE_ARRAY);
	g_value_take_boxed(gvalue, array);

	return TRUE;
}

zend_bool zval_to_gvalue(const zval *zvalue, GValue *gvalue)
{
	if (zvalue == NULL) {
		return FALSE;
	}

	TSRMLS_FETCH();

	switch (Z_TYPE_P(zvalue)) {
		case IS_NULL:
			g_value_init(gvalue, G_TYPE_NONE);
			break;

		case IS_BOOL:
			g_value_init(gvalue, G_TYPE_BOOLEAN);
			g_value_set_boolean(gvalue, Z_BVAL_P(zvalue));
			break;

		case IS_LONG:
			g_value_init(gvalue, G_TYPE_INT);
			g_value_set_int(gvalue, Z_LVAL_P(zvalue));
			break;

		case IS_DOUBLE:
			g_value_init(gvalue, G_TYPE_FLOAT);
			g_value_set_float(gvalue, (gfloat)Z_DVAL_P(zvalue));
			break;

		case IS_CONSTANT:
		case IS_STRING:
			g_value_init(gvalue, G_TYPE_STRING);
			g_value_set_string(gvalue, Z_STRVAL_P(zvalue));
			break;

		case IS_ARRAY:
		case IS_CONSTANT_ARRAY:
			return zhashtable_to_gvalue(Z_ARRVAL_P(zvalue), gvalue);
			break;

		case IS_OBJECT:
			if (!instanceof_function(Z_OBJCE_P(zvalue), gobject_ce_gobject TSRMLS_CC)) {
				// NOT GOBJECT. Don't know what to do with it
				php_error(E_WARNING, "This object is not GObject-derived. Can't convert");
				return FALSE;
			}

			gobject_gobject_object *php_gobject = __php_objstore_object(zvalue);

			if (!G_IS_OBJECT(php_gobject->gobject)) {
				php_error(E_WARNING, "Underlying object is not GObject. Shouldn't happen");
				return FALSE;
			}

			g_value_init(gvalue, G_TYPE_OBJECT);
			g_value_set_object(gvalue, G_OBJECT(php_gobject->gobject));

			break;

		case IS_RESOURCE:
			/* There's no way to handle resource gracefully */
			php_error(E_WARNING, "Can not convert resource to GObject");
			return FALSE;
			break;

		default:
			php_error(E_WARNING, "Unknown data type");
			return FALSE;
	}

	return TRUE;
}