/*----------------------------------------------------------------------------
  ChucK Concurrent, On-the-fly Audio Programming Language
    Compiler and Virtual Machine

  Copyright (c) 2004 Ge Wang and Perry R. Cook.  All rights reserved.
    http://chuck.stanford.edu/
    http://chuck.cs.princeton.edu/

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
  U.S.A.
-----------------------------------------------------------------------------*/

//-----------------------------------------------------------------------------
// file: chuck_oo.h
// desc: chuck baes objects
//
// author: Ge Wang (ge@ccrma.stanford.edu | gewang@cs.princeton.edu)
//         Ananya Misra (amisra@cs.princeton.edu)
//         Andrew Schran (aschran@princeton.edu) - fileIO implementation
// date: Autumn 2004
//-----------------------------------------------------------------------------
#ifndef __CHUCK_OO_H__
#define __CHUCK_OO_H__

#include "chuck_def.h"
#include "chuck_carrier.h"
#include <string>
#include <vector>
#include <map>
#include <queue>
#include <fstream>
#include <sstream> // REFACTOR-2017: for custom output
#ifndef __DISABLE_THREADS__
#include "util_thread.h" // added 1.3.0.0
#endif



#ifndef __PLATFORM_WIN32__
  #include <dirent.h>
#else
  struct DIR;
#endif

// forward reference
struct Chuck_Type;
struct Chuck_Value;
struct Chuck_Func;
struct Chuck_Namespace;
struct Chuck_Context;
struct Chuck_Env;
struct Chuck_VM_Code;
struct Chuck_VM_Shred;
struct Chuck_VM;
struct Chuck_IO_File;
class  CBufferSimple; // added 1.3.0.0




// enum
enum Chuck_Global_Get_Callback_Type
{
    ck_get_plain,
    ck_get_name,
    ck_get_id
};




//-----------------------------------------------------------------------------
// name: struct Chuck_VM_Object
// desc: base vm object
//-----------------------------------------------------------------------------
struct Chuck_VM_Object
{
public:
    Chuck_VM_Object() { this->init_ref(); }
    virtual ~Chuck_VM_Object() { }

public:
    // add reference (ge: april 2013: made these virtual)
    virtual void add_ref();
    // release reference
    virtual void release();
    // lock
    virtual void lock();

    // NOTE: be careful when overriding these, should always
    // explicitly call up to ChucK_VM_Object (ge: 2013)

public:
    // unlock_all: dis/allow deletion of locked objects
    static void lock_all();
    static void unlock_all();
    static t_CKBOOL our_locks_in_effect;

public:
    t_CKUINT m_ref_count; // reference count
    t_CKBOOL m_pooled; // if true, this allocates from a pool
    t_CKBOOL m_locked; // if true, this should never be deleted

public:
    // where
    std::vector<Chuck_VM_Object *> * m_v_ref;

private:
    void init_ref();
};




//-----------------------------------------------------------------------------
// name: struct Chuck_VM_Alloc
// desc: vm object manager
//-----------------------------------------------------------------------------
//struct Chuck_VM_Alloc
//{
//public:
//    static Chuck_VM_Alloc * instance();
//
//public:
//    void add_object( Chuck_VM_Object * obj );
//    void free_object( Chuck_VM_Object * obj );
//
//protected:
//    static Chuck_VM_Alloc * our_instance;
//
//protected:
//    Chuck_VM_Alloc();
//    ~Chuck_VM_Alloc();
//
//protected: // data
//    std::map<Chuck_VM_Object *, void *> m_objects;
//};




//-----------------------------------------------------------------------------
// name: struct Chuck_VTable
// desc: virtual table
//-----------------------------------------------------------------------------
struct Chuck_VTable
{
public:
    std::vector<Chuck_Func *> funcs;
};




//-----------------------------------------------------------------------------
// name: struct Chuck_Object
// dsec: base object
//-----------------------------------------------------------------------------
struct Chuck_Object : Chuck_VM_Object
{
public:
    Chuck_Object();
    virtual ~Chuck_Object();

public:
    // output current state (can be overridden)
    virtual void dump();

public:
    // output type info (can be overriden; but probably shouldn't be)
    virtual void help();

public:
    // virtual table
    Chuck_VTable * vtable;
    // reference to type
    Chuck_Type * type_ref;
    // the size of the data region
    t_CKUINT size;
    // data for the object
    t_CKBYTE * data;
};




// ISSUE: 64-bit (fixed 1.3.1.0)
#define CHUCK_ARRAY4_DATASIZE sz_INT
#define CHUCK_ARRAY8_DATASIZE sz_FLOAT
#define CHUCK_ARRAY16_DATASIZE sz_COMPLEX
#define CHUCK_ARRAY24_DATASIZE sz_VEC3 // 1.3.5.3
#define CHUCK_ARRAY32_DATASIZE sz_VEC4 // 1.3.5.3
#define CHUCK_ARRAY4_DATAKIND kindof_INT
#define CHUCK_ARRAY8_DATAKIND kindof_FLOAT
#define CHUCK_ARRAY16_DATAKIND kindof_COMPLEX
#define CHUCK_ARRAY24_DATAKIND kindof_VEC3 // 1.3.5.3
#define CHUCK_ARRAY32_DATAKIND kindof_VEC4 // 1.3.5.3
//-----------------------------------------------------------------------------
// name: struct Chuck_Array
// desc: native ChucK arrays ( virtual base class )
//-----------------------------------------------------------------------------
struct Chuck_Array : Chuck_Object
{
    // functionality that we can keep in common...

public:
    Chuck_Array() : m_array_type(NULL) { }
    virtual ~Chuck_Array();

    virtual t_CKINT size( ) = 0; // const { return m_size; } // array size
    virtual t_CKINT capacity( ) = 0; // const { return m_capacity; } // array capacity
    virtual t_CKINT set_size( t_CKINT size ) = 0; // array size
    virtual t_CKINT set_capacity( t_CKINT capacity ) = 0; // set
    virtual t_CKINT data_type_size( ) = 0; // size of stored type (from type_ref)
    virtual t_CKINT data_type_kind( ) = 0; // kind of stored type (from kindof)
    virtual t_CKINT find( const std::string & key ) = 0; // find
    virtual t_CKINT erase( const std::string & key ) = 0; // erase
    virtual void clear( ) = 0; // clear
    // get map keys | added (1.4.2.0) nshaheed
    virtual void get_keys( std::vector<std::string> & keys ) = 0;

    Chuck_Type * m_array_type;
};




//-----------------------------------------------------------------------------
// name: struct Chuck_Array4
// desc: native ChucK arrays (for 4-byte)
//-----------------------------------------------------------------------------
struct Chuck_Array4 : Chuck_Array
{
public:
    Chuck_Array4( t_CKBOOL is_obj, t_CKINT capacity = 8 );
    virtual ~Chuck_Array4();

public:
    t_CKUINT addr( t_CKINT i );
    t_CKUINT addr( const std::string & key );
    t_CKINT get( t_CKINT i, t_CKUINT * val );
    t_CKINT get( const std::string & key, t_CKUINT * val );
    t_CKINT set( t_CKINT i, t_CKUINT val );
    t_CKINT set( const std::string & key, t_CKUINT val );
    t_CKINT push_back( t_CKUINT val );
    t_CKINT pop_back( );
    t_CKINT pop_out( t_CKINT pos );
    t_CKINT back( t_CKUINT * val ) const;
    void    zero( t_CKUINT start, t_CKUINT end );

    virtual void    clear( );
    virtual t_CKINT size( ) { return m_vector.size(); }
    virtual t_CKINT capacity( ) { return m_vector.capacity(); }
    virtual t_CKINT set_size( t_CKINT size );
    virtual t_CKINT set_capacity( t_CKINT capacity );
    virtual t_CKINT find( const std::string & key );
    virtual t_CKINT erase( const std::string & key );
    virtual t_CKINT data_type_size( ) { return CHUCK_ARRAY4_DATASIZE; }
    virtual t_CKINT data_type_kind( ) { return CHUCK_ARRAY4_DATAKIND; }
    // get map keys | added (1.4.2.0) nshaheed
    virtual void get_keys( std::vector<std::string> & keys );

public:
    std::vector<t_CKUINT> m_vector;
    std::map<std::string, t_CKUINT> m_map;
    t_CKBOOL m_is_obj;
    // t_CKINT m_size;
    // t_CKINT m_capacity;
};




//-----------------------------------------------------------------------------
// name: struct Chuck_Array8
// desc: native ChucK arrays (for 8-byte)
//-----------------------------------------------------------------------------
struct Chuck_Array8 : Chuck_Array
{
public:
    Chuck_Array8( t_CKINT capacity = 8 );
    virtual ~Chuck_Array8();

public:
    t_CKUINT addr( t_CKINT i );
    t_CKUINT addr( const std::string & key );
    t_CKINT get( t_CKINT i, t_CKFLOAT * val );
    t_CKINT get( const std::string & key, t_CKFLOAT * val );
    t_CKINT set( t_CKINT i, t_CKFLOAT val );
    t_CKINT set( const std::string & key, t_CKFLOAT val );
    t_CKINT push_back( t_CKFLOAT val );
    t_CKINT pop_back( );
    t_CKINT pop_out( t_CKINT pos );
    t_CKINT back( t_CKFLOAT * val ) const;
    void    zero( t_CKUINT start, t_CKUINT end );

    virtual void    clear( );
    virtual t_CKINT size( ) { return m_vector.size(); }
    virtual t_CKINT capacity( ) { return m_vector.capacity(); }
    virtual t_CKINT set_size( t_CKINT size );
    virtual t_CKINT set_capacity( t_CKINT capacity );
    virtual t_CKINT find( const std::string & key );
    virtual t_CKINT erase( const std::string & key );
    virtual t_CKINT data_type_size( ) { return CHUCK_ARRAY8_DATASIZE; }
    virtual t_CKINT data_type_kind( ) { return CHUCK_ARRAY8_DATAKIND; }
    // get map keys | added (1.4.2.0) nshaheed
    virtual void get_keys( std::vector<std::string> & keys );

public:
    std::vector<t_CKFLOAT> m_vector;
    std::map<std::string, t_CKFLOAT> m_map;
    // t_CKINT m_size;
    // t_CKINT m_capacity;
};




//-----------------------------------------------------------------------------
// name: struct Chuck_Array16
// desc: native ChucK arrays (for 16-byte)
//-----------------------------------------------------------------------------
struct Chuck_Array16 : Chuck_Array
{
public:
    Chuck_Array16( t_CKINT capacity = 8 );
    virtual ~Chuck_Array16();

public:
    t_CKUINT addr( t_CKINT i );
    t_CKUINT addr( const std::string & key );
    t_CKINT get( t_CKINT i, t_CKCOMPLEX * val );
    t_CKINT get( const std::string & key, t_CKCOMPLEX * val );
    t_CKINT set( t_CKINT i, const t_CKCOMPLEX & val );
    t_CKINT set( const std::string & key, const t_CKCOMPLEX & val );
    t_CKINT push_back( const t_CKCOMPLEX & val );
    t_CKINT pop_back( );
    t_CKINT pop_out( t_CKINT pos );
    t_CKINT back( t_CKCOMPLEX * val ) const;
    void    zero( t_CKUINT start, t_CKUINT end );

    virtual void    clear( );
    virtual t_CKINT size( ) { return m_vector.size(); }
    virtual t_CKINT capacity( ) { return m_vector.capacity(); }
    virtual t_CKINT set_size( t_CKINT size );
    virtual t_CKINT set_capacity( t_CKINT capacity );
    virtual t_CKINT find( const std::string & key );
    virtual t_CKINT erase( const std::string & key );
    virtual t_CKINT data_type_size( ) { return CHUCK_ARRAY16_DATASIZE; }
    virtual t_CKINT data_type_kind( ) { return CHUCK_ARRAY16_DATAKIND; }
    // get map keys | added (1.4.2.0) nshaheed
    virtual void get_keys( std::vector<std::string> & keys );

public:
    std::vector<t_CKCOMPLEX> m_vector;
    std::map<std::string, t_CKCOMPLEX> m_map;
    // t_CKINT m_size;
    // t_CKINT m_capacity;
};




//-----------------------------------------------------------------------------
// name: struct Chuck_Array24
// desc: native ChucK arrays (for vec3)
//-----------------------------------------------------------------------------
struct Chuck_Array24 : Chuck_Array
{
public:
    Chuck_Array24( t_CKINT capacity = 8 );
    virtual ~Chuck_Array24();

public:
    t_CKUINT addr( t_CKINT i );
    t_CKUINT addr( const std::string & key );
    t_CKINT get( t_CKINT i, t_CKVEC3 * val );
    t_CKINT get( const std::string & key, t_CKVEC3 * val );
    t_CKINT set( t_CKINT i, const t_CKVEC3 & val );
    t_CKINT set( const std::string & key, const t_CKVEC3 & val );
    t_CKINT push_back( const t_CKVEC3 & val );
    t_CKINT pop_back( );
    t_CKINT pop_out( t_CKUINT pos );
    t_CKINT back( t_CKVEC3 * val ) const;
    void    zero( t_CKUINT start, t_CKUINT end );

    virtual void    clear( );
    virtual t_CKINT size( ) { return m_vector.size(); }
    virtual t_CKINT capacity( ) { return m_vector.capacity(); }
    virtual t_CKINT set_size( t_CKINT size );
    virtual t_CKINT set_capacity( t_CKINT capacity );
    virtual t_CKINT find( const std::string & key );
    virtual t_CKINT erase( const std::string & key );
    virtual t_CKINT data_type_size( ) { return CHUCK_ARRAY24_DATASIZE; }
    virtual t_CKINT data_type_kind( ) { return CHUCK_ARRAY24_DATAKIND; }
    // get map keys | added (1.4.2.0) nshaheed
    virtual void get_keys( std::vector<std::string> & keys );

public:
    std::vector<t_CKVEC3> m_vector;
    std::map<std::string, t_CKVEC3> m_map;
};




//-----------------------------------------------------------------------------
// name: struct Chuck_Array32
// desc: native ChucK arrays (for vec4)
//-----------------------------------------------------------------------------
struct Chuck_Array32 : Chuck_Array
{
public:
    Chuck_Array32( t_CKINT capacity = 8 );
    virtual ~Chuck_Array32();

public:
    t_CKUINT addr( t_CKINT i );
    t_CKUINT addr( const std::string & key );
    t_CKINT get( t_CKINT i, t_CKVEC4 * val );
    t_CKINT get( const std::string & key, t_CKVEC4 * val );
    t_CKINT set( t_CKINT i, const t_CKVEC4 & val );
    t_CKINT set( const std::string & key, const t_CKVEC4 & val );
    t_CKINT push_back( const t_CKVEC4 & val );
    t_CKINT pop_back( );
    t_CKINT pop_out( t_CKUINT pos );
    t_CKINT back( t_CKVEC4 * val ) const;
    void    zero( t_CKUINT start, t_CKUINT end );

    virtual void    clear( );
    virtual t_CKINT size( ) { return m_vector.size(); }
    virtual t_CKINT capacity( ) { return m_vector.capacity(); }
    virtual t_CKINT set_size( t_CKINT size );
    virtual t_CKINT set_capacity( t_CKINT capacity );
    virtual t_CKINT find( const std::string & key );
    virtual t_CKINT erase( const std::string & key );
    virtual t_CKINT data_type_size( ) { return CHUCK_ARRAY32_DATASIZE; }
    virtual t_CKINT data_type_kind( ) { return CHUCK_ARRAY32_DATAKIND; }
    // get map keys | added (1.4.1.2) nshaheed
    virtual void get_keys( std::vector<std::string> & keys );

public:
    std::vector<t_CKVEC4> m_vector;
    std::map<std::string, t_CKVEC4> m_map;
};




//-----------------------------------------------------------------------------
// name: Chuck_Global_Event_Listener
// desc: base Listener struct
//-----------------------------------------------------------------------------
struct Chuck_Global_Event_Listener
{
    union {
        void (* void_callback)(void);
        void (* named_callback)(const char *);
        void (* id_callback)(t_CKINT);
    };
    t_CKBOOL listen_forever;
    Chuck_Global_Get_Callback_Type callback_type;
    std::string name;
    t_CKINT id;
    Chuck_Global_Event_Listener() : void_callback(NULL), listen_forever(FALSE),
        callback_type(ck_get_plain), name(""), id(0) {};
};




//-----------------------------------------------------------------------------
// name: Chuck_Event
// desc: base Chuck Event class
//-----------------------------------------------------------------------------
struct Chuck_Event : Chuck_Object
{
public:
    // signal/broadcast "local" -- signal ChucK Events
    void signal_local();
    void broadcast_local();
    void wait( Chuck_VM_Shred * shred, Chuck_VM * vm );
    t_CKBOOL remove( Chuck_VM_Shred * shred );

public:
    // 1.4.0.0 (jack): global events -- signal/broadcast code running
    // externally, elsewhere in the host
    void signal_global();
    void broadcast_global();

    // 1.4.1.0 (jack): global events multiple callbacks.
    // can listen with a callback(), callback( event name ), or callback( id )
    void global_listen( void (* cb)(void), t_CKBOOL listen_forever );
    void global_listen( std::string name, void (* cb)(const char *), t_CKBOOL listen_forever );
    void global_listen( t_CKINT id, void (* cb)(t_CKINT), t_CKBOOL listen_forever );
    t_CKBOOL remove_listen( void (* cb)(void) );
    t_CKBOOL remove_listen( std::string name, void (* cb)(const char *) );
    t_CKBOOL remove_listen( t_CKINT id, void (* cb)(t_CKINT) );

public: // internal
    // added 1.3.0.0: queue_broadcast now takes event_buffer
    void queue_broadcast( CBufferSimple * event_buffer = NULL );

public:
    static t_CKUINT our_can_wait;

protected:
    std::queue<Chuck_VM_Shred *> m_queue;
    #ifndef __DISABLE_THREADS__
    // 1.4.1.0 (ge/jack) TODO: rewrite queue_broadcast to use a lock-free queue
    // and avoid the use of a lock in events
    XMutex m_queue_lock;
    #endif
    std::queue<Chuck_Global_Event_Listener> m_global_queue;
};




//-----------------------------------------------------------------------------
// name: Chuck_String
// desc: base Chuck string class
//-----------------------------------------------------------------------------
struct Chuck_String : Chuck_Object
{
public:
    // constructor
    Chuck_String( const std::string & s = "" ) { set( s ); }
    // destructor
    ~Chuck_String() { }

    // set string (makes copy)
    void set( const std::string & s ) { m_str = s; m_charptr = m_str.c_str(); }
    // get as standard c++ string
    const std::string & str() { return m_str; }
    // get as C string (NOTE: use this in dynamical modules like chugins!)
    const char * c_str() { return m_charptr; }

private:
    // c pointer | HACK: needed for ensure string passing in dynamic modules
    const char * m_charptr; // REFACTOR-2017
    // c++ string
    std::string m_str;
};




//-----------------------------------------------------------------------------
// name: Chuck_IO
// desc: base Chuck IO class
//-----------------------------------------------------------------------------
struct Chuck_IO : Chuck_Event
{
public:
    Chuck_IO();
    virtual ~Chuck_IO();

public:
    // meta
    virtual t_CKBOOL good() = 0;
    virtual void close() = 0;
    virtual void flush() = 0;
    virtual t_CKINT mode() = 0;
    virtual void mode( t_CKINT flag ) = 0;

    // reading
    virtual Chuck_String * readLine() = 0;
    virtual t_CKINT readInt( t_CKINT flags ) = 0;
    virtual t_CKFLOAT readFloat() = 0;
    virtual t_CKBOOL readString( std::string & str ) = 0;
    virtual t_CKBOOL eof() = 0;

    // writing
    virtual void write( const std::string & val ) = 0;
    virtual void write( t_CKINT val ) = 0;
    virtual void write( t_CKINT val, t_CKINT flags ) = 0;
    virtual void write( t_CKFLOAT val ) = 0;

    // constants
public:
    static const t_CKINT INT32;
    static const t_CKINT INT16;
    static const t_CKINT INT8;

    // asynchronous I/O members
    static const t_CKINT MODE_SYNC;
    static const t_CKINT MODE_ASYNC;
    Chuck_Event * m_asyncEvent;
    #ifndef __DISABLE_THREADS__
    XThread * m_thread;
    #endif
    struct async_args
    {
        Chuck_IO_File * fileio_obj;
        void *RETURN; // actually a Chuck_DL_Return
        t_CKINT intArg;
        t_CKFLOAT floatArg;
        std::string stringArg;
    };
};



#ifndef __DISABLE_FILEIO__
//-----------------------------------------------------------------------------
// name: Chuck_IO_File
// desc: Chuck File IO class
//-----------------------------------------------------------------------------
struct Chuck_IO_File : Chuck_IO
{
public:
    // REFACTOR-2017
    Chuck_IO_File( Chuck_VM * vm );
    virtual ~Chuck_IO_File();

public:
    // meta
    virtual t_CKBOOL open( const std::string & path, t_CKINT flags );
    virtual t_CKBOOL good();
    virtual void close();
    virtual void flush();
    virtual t_CKINT mode();
    virtual void mode( t_CKINT flag );
    virtual t_CKINT size();

    // seeking
    virtual void seek( t_CKINT pos );
    virtual t_CKINT tell();

    // directories
    virtual t_CKINT isDir();
    virtual Chuck_Array4 * dirList();

    // reading
    // virtual Chuck_String * read( t_CKINT length );
    virtual Chuck_String * readLine();
    virtual t_CKINT readInt( t_CKINT flags );
    virtual t_CKFLOAT readFloat();
    virtual t_CKBOOL readString( std::string & str );
    virtual t_CKBOOL eof();

    // reading -- async
    /* TODO: doesn't look like asynchronous reads will work
     static THREAD_RETURN ( THREAD_TYPE read_thread ) ( void *data );
     static THREAD_RETURN ( THREAD_TYPE readLine_thread ) ( void *data );
     static THREAD_RETURN ( THREAD_TYPE readInt_thread ) ( void *data );
     static THREAD_RETURN ( THREAD_TYPE readFloat_thread ) ( void *data );
     */

    // writing
    virtual void write( const std::string & val );
    virtual void write( t_CKINT val );
    virtual void write( t_CKINT val, t_CKINT flags );
    virtual void write( t_CKFLOAT val );

    #ifndef __DISABLE_THREADS__
    // writing -- async
    static THREAD_RETURN ( THREAD_TYPE writeStr_thread ) ( void *data );
    static THREAD_RETURN ( THREAD_TYPE writeInt_thread ) ( void *data );
    static THREAD_RETURN ( THREAD_TYPE writeFloat_thread ) ( void *data );
    #endif

public:
    // constants
    static const t_CKINT FLAG_READ_WRITE;
    static const t_CKINT FLAG_READONLY;
    static const t_CKINT FLAG_WRITEONLY;
    static const t_CKINT FLAG_APPEND;
    static const t_CKINT TYPE_ASCII;
    static const t_CKINT TYPE_BINARY;

protected:
    // open flags
    t_CKINT m_flags;
    // I/O mode
    t_CKINT m_iomode;
    // file stream
    std::fstream m_io;
    // directory pointer
    DIR * m_dir;
    // directory location
    long m_dir_start;
    // path
    std::string m_path;
    // vm and shred
    Chuck_VM * m_vmRef;
};
#endif // __DISABLE_FILEIO__




//-----------------------------------------------------------------------------
// name: Chuck_IO_Chout
// desc: Chuck console IO out
//-----------------------------------------------------------------------------
struct Chuck_IO_Chout : Chuck_IO
{
public:
    Chuck_IO_Chout( Chuck_Carrier * carrier );
    virtual ~Chuck_IO_Chout();

public:
    // meta
    virtual t_CKBOOL good();
    virtual void close();
    virtual void flush();
    virtual t_CKINT mode();
    virtual void mode( t_CKINT flag );

    // reading
    virtual Chuck_String * readLine();
    virtual t_CKINT readInt( t_CKINT flags );
    virtual t_CKFLOAT readFloat();
    virtual t_CKBOOL readString( std::string & str );
    virtual t_CKBOOL eof();

    // writing
    virtual void write( const std::string & val );
    virtual void write( t_CKINT val );
    virtual void write( t_CKINT val, t_CKINT flags );
    virtual void write( t_CKFLOAT val );

public: // REFACTOR-2017
    // set callback
    void set_output_callback( void (* fp)(const char *) );

private:
    // callback
    void (* m_callback)(const char *);
    // intermediate line storage
    std::stringstream m_buffer;
};




//-----------------------------------------------------------------------------
// name: Chuck_IO_Cherr
// desc: Chuck console IO err
//-----------------------------------------------------------------------------
struct Chuck_IO_Cherr : Chuck_IO
{
public:
    Chuck_IO_Cherr( Chuck_Carrier * carrier );
    virtual ~Chuck_IO_Cherr();

public:
    // meta
    virtual t_CKBOOL good();
    virtual void close();
    virtual void flush();
    virtual t_CKINT mode();
    virtual void mode( t_CKINT flag );

    // reading
    virtual Chuck_String * readLine();
    virtual t_CKINT readInt( t_CKINT flags );
    virtual t_CKFLOAT readFloat();
    virtual t_CKBOOL readString( std::string & str );
    virtual t_CKBOOL eof();

    // writing
    virtual void write( const std::string & val );
    virtual void write( t_CKINT val );
    virtual void write( t_CKINT val, t_CKINT flags );
    virtual void write( t_CKFLOAT val );

public:
    // set callback | REFACTOR-2017
    void set_output_callback( void (* fp)(const char *) );

private:
    // callback
    void (* m_callback)(const char *);
    // intermediate line storage
    std::stringstream m_buffer;
};




#endif
