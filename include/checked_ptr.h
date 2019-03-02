#pragma once

#include <memory>

#include "checked_ptr_crc32.h"

#ifdef _WIN32
#include <Windows.h>
#else
#include <fcntl.h>
#include <unistd.h>
#endif

#ifdef CHECKED_PTR_USE_TYPEID_HASH
#include <typeinfo>
#endif

#define CHECKED_OBJ( tname, ... )                                                        \
    checkable_obj< tname >( CONSTEXPR_TYPENAME_HASH( tname ), __VA_ARGS__ )

#define MAKE_CHECKED( type, ptr ) checked_ptr< type >( ptr )

#define DO_CHECK( type, ptr ) ptr.valid( CONSTEXPR_TYPENAME_HASH( type ) )

#ifdef _WIN32

// sketchy af
bool addr_is_readable( void* mem ) {
    MEMORY_BASIC_INFORMATION info;
    auto s = VirtualQuery( mem, &info, sizeof( info ) );
    return info.Protect >= PAGE_READONLY;
}

#else

// even more sketchy
bool addr_is_readable( void* mem ) {
    int written = write( STDOUT_FILENO, mem, 4 );
    return written > 0;
}

#endif

// #define CHECKED_PTR_USE_TEMPLATE_HASH

template < typename T >
struct checkable_obj {

    uint32_t validator_field_;

    T payload_;

#ifndef CHECKED_PTR_USE_TYPEID_HASH

    template < typename... Args >
    checkable_obj( uint32_t&& typename_hash, Args&&... args )
        : validator_field_( typename_hash )
        , payload_( std::forward< Args >( args )... ) {}

#else

    template < typename... Args >
    checkable_obj( Args&&... args )
        : validator_field_( typeid( T ).hash_code() )
        , payload_( std::forward< Args >( args )... ) {}

#endif

    T& operator*() { return payload_; }

    T* operator->() { return &payload_; }

    ~checkable_obj() { validator_field_ = 0; }
};

// #define CHECKED_PTR_USE_TYPEID_HASH

template < typename T >
class checked_ptr {
  public:
    checkable_obj< T >* memory_region_;

    checked_ptr( void* mem )
        : memory_region_( reinterpret_cast< checkable_obj< T >* >( mem ) ) {}

    checked_ptr( long long mem )
        : memory_region_( reinterpret_cast< checkable_obj< T >* >( mem ) ) {}

#ifndef CHECKED_PTR_USE_TYPEID_HASH
    bool check( uint32_t typename_hash ) const noexcept {
        return readable() && check_type( typename_hash );
    }
#else
    bool check() const noexcept { return readable() && check_type(); }
#endif

    bool readable() const {
        return addr_is_readable(
            const_cast< void* >( reinterpret_cast< const void* >( memory_region_ ) ) );
    }

#ifndef CHECKED_PTR_USE_TYPEID_HASH
    bool check_type( uint32_t typename_hash ) const {
        return memory_region_->validator_field_ == typename_hash;
    }
#else
    bool check_type() const {
        return memory_region_->validator_field_ == typeid( T ).hash_code();
    }
#endif

    T& operator*() { return ( *( *memory_region_ ) ); }

    T* operator->() { return &( *( *memory_region_ ) ); }
};

#ifndef CHECKED_PTR_USE_TYPEID_HASH

template < typename T >
checked_ptr< T > make_checked( void* mem, size_t check_hash ) {
    auto ptr = checked_ptr< T >( mem );
    if ( !ptr.check( check_hash ) )
        throw std::runtime_error( "invalid ptr" );
    return ptr;
}

#else

template < typename T >
checked_ptr< T > make_checked( void* mem ) {
    auto ptr = checked_ptr< T >( mem );
    if ( !ptr.check() )
        throw std::runtime_error( "invalid ptr" );
    return ptr;
}

#endif

#define GEN_CHECKED_PTR( type, name, mem )                                               \
    auto name = checked_ptr< type >( mem );                                              \
    if ( !name.check( CONSTEXPR_TYPENAME_HASH( type ) ) )                                \
        throw std::runtime_error( "invalid pointer" );
