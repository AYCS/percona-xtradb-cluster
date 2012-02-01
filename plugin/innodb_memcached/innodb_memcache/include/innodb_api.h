/***********************************************************************

Copyright (c) 2010, Oracle and/or its affiliates. All Rights Reserved.

This program is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by the
Free Software Foundation; version 2 of the License.

This program is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
59 Temple Place, Suite 330, Boston, MA 02111-1307 USA

***********************************************************************/
/**************************************************//**
@file innodb_api.h

Created 03/15/2011      Jimmy Yang
*******************************************************/

#ifndef innodb_api_h
#define innodb_api_h

#include "api0api.h"
#include "innodb_engine.h"
#include "assert.h"
#include "handler_api.h"

/** Macros to lock/unlock the engine connection mutex */
#define LOCK_CONN_IF_NOT_LOCKED(has_lock, engine)	\
	if (!(has_lock)) {				\
		pthread_mutex_lock(&engine->conn_mutex);\
	}

#define UNLOCK_CONN_IF_NOT_LOCKED(has_lock, engine)	\
	if (!(has_lock)) {				\
		pthread_mutex_unlock(&engine->conn_mutex);\
	}

/** mci_column is the structure stores and describes a column info in InnoDB
Memcached. The column type we need to support in the memcached key value
store is either of character type of integer type (see following "enum
mci_item_idx" for columns supporting memcached) */
typedef struct mci_column {
	char*		m_str;		/*!< char value of the column */
	int		m_len;		/*!< char value length */
	uint64_t	m_digit;	/*!< integer value */
	bool		m_is_str;	/*!< whether the value is char or int */
	bool		m_enabled;	/*!< valid column value */
	bool		m_is_null;	/*!< whether it is a NULL value */
	bool		m_allocated;	/*!< whether memory allocated to store
					the value */
} mci_column_t;

/** We would need to fetch 5 column values from each key value rows if they
are available. They are the "key", "value", "cas", "exp" and "flag" */
#define	MCI_ITEM_TO_GET		5

enum mci_item_idx {
	MCI_COL_KEY,
	MCI_COL_VALUE,
	MCI_COL_FLAG,
	MCI_COL_CAS,
	MCI_COL_EXP
};

/** mci_item_t describes columns from a row we fetched from a memcached
mapped InnoDB table */
typedef struct mci_items {
	mci_column_t	mci_item[MCI_ITEM_TO_GET]; /*!< columns in a row */
	mci_column_t*	mci_add_value;		/*!< whether there will be
						additional/multiple values
						to be stored */
	int		mci_add_num;		/*!< number of additional
						value columns */
} mci_item_t;

/*************************************************************//**
Register InnoDB Callback functions */
void
register_innodb_cb(
/*===============*/
	char*	p);		/*!<in: Pointer to callback function arrary */

/*********************************************************************
Open a table and return a cursor for the table. */
ib_err_t
innodb_api_begin(
/*=============*/
	innodb_engine_t*engine,		/*!< in: InnoDB Memcached engine */
	const char*	dbname,		/*!< in: database name */
	const char*	name,		/*!< in: table name */
	innodb_conn_data_t* conn_data,
					/*!< in: connnection specific data */
	ib_trx_t	ib_trx,		/*!< in: transaction */
	ib_crsr_t*	crsr,		/*!< out: innodb cursor */
	ib_crsr_t*	idx_crsr,	/*!< out: innodb index cursor */
	ib_lck_mode_t	lock_mode);	/*!< in:  lock mode */

/*************************************************************//**
Position a row accord to key, and fetch value if needed
@return DB_SUCCESS if successful otherwise, error code */
ib_err_t
innodb_api_search(
/*==============*/
	innodb_engine_t*	engine,	/*!< in: InnoDB Memcached engine */
	innodb_conn_data_t*	cursor_data,/*!< in: cursor info */
	ib_crsr_t*		crsr,	/*!< in/out: cursor used to seacrh */
	const char*		key,	/*!< in: key to search */
	int			len,	/*!< in: key length */
	mci_item_t*		item,	/*!< in: result */
	ib_tpl_t*		r_tpl,	/*!< in: tpl for other DML operations */
	bool			sel_only); /*!< in: for select only */

/*************************************************************//**
Insert a row
@return DB_SUCCESS if successful otherwise, error code */
ib_err_t
innodb_api_insert(
/*==============*/
	innodb_engine_t*	engine,	/*!< in: InnoDB Memcached engine */
	innodb_conn_data_t*	cursor_data,/*!< in: cursor info */
	const char*		key,	/*!< in: value to insert */
	int			len,	/*!< in: value length */
	uint32_t		val_len,/*!< in: value length */
	uint64_t		exp,	/*!< in: expire time */
	uint64_t*		cas,	/*!< in/out: cas value */
        uint64_t		flags);	/*!< in: flags */

/*************************************************************//**
Delete a row, implements the "remove" command
@return ENGINE_SUCCESS if successful otherwise, error code */
ENGINE_ERROR_CODE
innodb_api_delete(
/*==============*/
	innodb_engine_t*	engine,	/*!< in: InnoDB Memcached engine */
	innodb_conn_data_t*	cursor_data,/*!< in: cursor info */
	const char*		key,	/*!< in: value to insert */
	int			len);	/*!< in: value length */

/*************************************************************//**
Update a row with arithmetic operation
@return DB_SUCCESS if successful otherwise ENGINE_NOT_STORED*/
ENGINE_ERROR_CODE
innodb_api_arithmetic(
/*==================*/
	innodb_engine_t*	engine,	/*!< in: InnoDB Memcached engine */
	innodb_conn_data_t*	cursor_data,/*!< in: cursor info */
	const char*		key,	/*!< in: key values */
	int			len,	/*!< in: key length */
	int			delta,	/*!< in: value to add or subtract */
	bool			increment,/*!< in: increment or decrement */
	uint64_t*		cas,	/*!< out: cas */
	rel_time_t		exp_time, /*!< in: expire time */
	bool			create,	/*!< in: whether to create new entry
					if not found */
	uint64_t		initial,/*!< in: initialize value */
	uint64_t*		result);/*!< out: result value */

/*************************************************************//**
This is the interface to following commands:
	1) add
	2) replace
	3) append
	4) prepend
	5) set
	6) cas
@return ENGINE_SUCCESS if successful otherwise, error code */
ENGINE_ERROR_CODE
innodb_api_store(
/*=============*/
	innodb_engine_t*	engine,	/*!< in: InnoDB Memcached engine */
	innodb_conn_data_t*	cursor_data,/*!< in: cursor info */
	const char*		key,	/*!< in: key value */
	int			len,	/*!< in: key length */
	uint32_t		val_len,/*!< in: value length */
	uint64_t		exp,	/*!< in: expire time */
	uint64_t*		cas,	/*!< out: cas value */
	uint64_t		input_cas,/*!< in: cas value supplied by user */
	uint64_t		flags,	/*!< in: flags */
	ENGINE_STORE_OPERATION	op);	/*!< in: Operations */

/*********************************************************************
Implement the "flush_all" command, map to InnoDB's trunk table operation
return ENGINE_SUCCESS is all successful */
ENGINE_ERROR_CODE
innodb_api_flush(
/*=============*/
	innodb_engine_t*	engine,	/*!< in: InnoDB Memcached engine */
	const char*		dbname,	/*!< in: database name */
	const char*		name);	/*!< in: table name */

/*************************************************************//**
Get current time */
uint64_t
mci_get_time(void);
/*==============*/

/** types of operations performed */
typedef enum conn_op_type {
	CONN_OP_READ,
	CONN_OP_WRITE,
	CONN_OP_DELETE,
	CONN_OP_FLUSH
} op_type_t;

/*************************************************************//**
Increment read and write counters, if they exceed the batch size,
commit the transaction. */
void
innodb_api_cursor_reset(
/*====================*/
	innodb_engine_t*	engine,		 /*!< in: InnoDB Memcached
						engine */
	innodb_conn_data_t*	conn_data,	/*!< in/out: cursor affiliated
						with a connection */
	op_type_t               op_type);	/*!< in: type of DML
						performed */

/*************************************************************//**
Following are a set of InnoDB callback function wrappers for functions
that will be used outside innodb_api.c
@return DB_SUCCESS if successful or error code */
ib_err_t
innodb_cb_cursor_close(
/*===================*/
	ib_crsr_t	ib_crsr);	/*!< in: cursor to close */

/*************************************************************//**
Start a transaction
@return DB_SUCCESS if successful or error code */
ib_trx_t
innodb_cb_trx_begin(
/*================*/
	ib_trx_level_t	ib_trx_level);	/*!< in: trx isolation level */

/*************************************************************//**
Commit the transaction
@return DB_SUCCESS if successful or error code */
ib_err_t
innodb_cb_trx_commit(
/*=================*/
	ib_trx_t	ib_trx);	/*!< in: transaction to commit */

/*************************************************************//**
Close table associated to the connection
@return DB_SUCCESS if successful or error code */
ib_err_t
innodb_cb_close_thd(
/*=================*/
	void*		thd);		/*!< in: THD */

/*****************************************************************//**
update the cursor with new transactions and also reset the cursor
@return DB_SUCCESS or error code */
ib_err_t
innodb_cb_cursor_new_trx(
/*=====================*/
	ib_crsr_t	ib_crsr,	/*!< in/out: InnoDB cursor */
	ib_trx_t	ib_trx);	/*!< in: transaction */

/*****************************************************************//**
Lock the table with specified lock mode
@return DB_SUCCESS or error code */
ib_err_t
innodb_cb_cursor_lock(
/*==================*/
	ib_crsr_t	ib_crsr,	/*!< in/out: cursor on the table */
	ib_lck_mode_t	ib_lck_mode);	/*!< in: lock mode */

/*****************************************************************//**
Create an InnoDB tuple used for index/table search.
@return own: Tuple for current index */
ib_tpl_t
innodb_cb_read_tuple_create(
/*========================*/
	ib_crsr_t	ib_crsr);	/*!< in: Cursor instance */

/*****************************************************************//**
Move cursor to the first record in the table.
@return DB_SUCCESS or err code */
ib_err_t
innodb_cb_cursor_first(
/*===================*/
	ib_crsr_t	ib_crsr);	/*!< in: InnoDB cursor instance */

/*****************************************************************//**
Read current row.
@return DB_SUCCESS or err code */
ib_err_t
innodb_cb_read_row(
/*===============*/
	ib_crsr_t	ib_crsr,	/*!< in: InnoDB cursor instance */
	ib_tpl_t	ib_tpl);	/*!< out: read cols into this tuple */

/*****************************************************************//**
Get a column type, length and attributes from the tuple.
@return len of column data */
ib_ulint_t
innodb_cb_col_get_meta(
/*===================*/
	ib_tpl_t	ib_tpl,		/*!< in: tuple instance */
	ib_ulint_t	i,		/*!< in: column index in tuple */
	ib_col_meta_t*	ib_col_meta);	/*!< out: column meta data */

/*****************************************************************//**
Destroy an InnoDB tuple. */
void
innodb_cb_tuple_delete(
/*===================*/
        ib_tpl_t	ib_tpl);	/*!< in,own: Tuple instance to delete */

/*****************************************************************//**
Return the number of columns in the tuple definition.
@return number of columns */
ib_ulint_t
innodb_cb_tuple_get_n_cols(
/*=======================*/
	const ib_tpl_t	ib_tpl);	/*!< in: Tuple for table/index */

/*****************************************************************//**
Get a column value pointer from the tuple.
@return NULL or pointer to buffer */
const void*
innodb_cb_col_get_value(
/*====================*/
	ib_tpl_t	ib_tpl,		/*!< in: tuple instance */
	ib_ulint_t	i);		/*!< in: column index in tuple */

/********************************************************************//**
Open a table using the table name.
@return table instance if found */
ib_err_t
innodb_cb_open_table(
/*=================*/
	const char*	name,		/*!< in: table name to lookup */
	ib_trx_t	ib_trx,		/*!< in: transaction */
	ib_crsr_t*	ib_crsr);	/*!< in: cursor to be used */

/*****************************************************************//**
Get a column name from the tuple.
@return name of the column */
char*
innodb_cb_col_get_name(
/*===================*/
	ib_crsr_t	ib_crsr,	/*!< in: InnoDB cursor instance */
	ib_ulint_t	i);		/*!< in: column index in tuple */

/*****************************************************************//**
Open an InnoDB secondary index cursor and return a cursor handle to it.
@return DB_SUCCESS or err code */
ib_err_t
innodb_cb_cursor_open_index_using_name(
/*===================================*/
	ib_crsr_t	ib_open_crsr,	/*!< in: open/active cursor */
	const char*	index_name,	/*!< in: secondary index name */
	ib_crsr_t*	ib_crsr,	/*!< out,own: InnoDB index cursor */
	int*		idx_type,	/*!< out: index is cluster index */
	ib_id_u64_t*	idx_id);	/*!< out: index id */

/*****************************************************************//**
Check whether the binlog option is turned on
(innodb_direct_access_enable_binlog)
@return TRUE if on */
bool
innodb_cb_binlog_enabled();
/*======================*/

#endif
