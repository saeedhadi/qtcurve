/*****************************************************************************
 *   Copyright 2013 - 2014 Yichao Yu <yyc1992@gmail.com>                     *
 *                                                                           *
 *   This program is free software; you can redistribute it and/or modify    *
 *   it under the terms of the GNU Lesser General Public License as          *
 *   published by the Free Software Foundation; either version 2.1 of the    *
 *   License, or (at your option) version 3, or any later version accepted   *
 *   by the membership of KDE e.V. (or its successor approved by the         *
 *   membership of KDE e.V.), which shall act as a proxy defined in          *
 *   Section 6 of version 3 of the license.                                  *
 *                                                                           *
 *   This program is distributed in the hope that it will be useful,         *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of          *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU       *
 *   Lesser General Public License for more details.                         *
 *                                                                           *
 *   You should have received a copy of the GNU Lesser General Public        *
 *   License along with this library. If not,                                *
 *   see <http://www.gnu.org/licenses/>.                                     *
 *****************************************************************************/

#ifndef _QTC_UTILS_UTILS_H_
#define _QTC_UTILS_UTILS_H_

#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#ifndef __cplusplus
#  include <stdbool.h>
#endif

#include "macros.h"

/**
 * \file utils.h
 * \author Yichao Yu <yyc1992@gmail.com>
 * \brief Some generic functions and macros.
 */

/**
 * Allocate memory and initialize it to zero.
 * \param size size of the memory
 */
QTC_ALWAYS_INLINE static inline void*
qtcAlloc0(size_t size)
{
    void *p = malloc(size);
    memset(p, 0, size);
    return p;
}

/**
 * Free a pointer if it is not NULL.
 * \param p the pointer to be freed.
 */
QTC_ALWAYS_INLINE static inline void
qtcFree(void *p)
{
    if (p) {
        free(p);
    }
}

/**
 * Allocate memory of size \param size for a \param type.
 * The memory is initialized to zero.
 * \param type type pointed to by the pointer
 * \param size size of the memory
 * \sa qtcAlloc0
 */
#define qtcNewSize(type, size) ((type*)qtcAlloc0(size))

/**
 * Allocate memory for a \param type or an array of \param type.
 * The memory is initialized to zero.
 * \param type type pointed to by the pointer
 * \param n (optional) elements in the array (default: 1)
 * \sa qtcNewSize
 */
#define qtcNew(type, n...)                              \
    qtcNewSize(type, sizeof(type) * (QTC_DEFAULT(n, 1)))

#ifdef __cplusplus
namespace QtCurve {
/**
 * Turn a variable into a const reference. This is useful for range-based for
 * loop where a non-const variable may cause unnecessary copy.
 */
template <class T>
static inline const T&
const_(const T &t)
{
    return t;
}
}
#endif

/**
 * Define a buffer type (struct) for \param type.
 * \sa QTC_DEF_LOCAL_BUFF
 */
#define QTC_BUFF_TYPE(type)                     \
    struct {                                    \
        union {                                 \
            type *p;                            \
            void *_p;                           \
        };                                      \
        size_t l;                               \
        type *const static_p;                   \
        const size_t static_l;                  \
    }

/**
 * \brief Define a local buffer for holding an array of \param type.
 * \param type type of the array element.
 * \param name name of the variable
 * \param stack_size the maximum number of elements in the array that will be
 *                   kept on the stack.
 * \param size real size of the array.
 *
 * This macro define a buffer \param name for an array of \param type.
 * The buffer's field p points to the address of the array.
 * Use QTC_FREE_LOCAL_BUFF to free the buffer.
 * \sa QTC_FREE_LOCAL_BUFF
 * \sa QTC_RESIZE_LOCAL_BUFF
 */
#define QTC_DEF_LOCAL_BUFF(type, name, stack_size, size)                \
    type __##qtc_local_buff##name[stack_size];                          \
    QTC_BUFF_TYPE(type) name = {                                        \
        {__##qtc_local_buff##name},                                     \
        sizeof(__##qtc_local_buff##name) / sizeof(type),                \
        __##qtc_local_buff##name,                                       \
        sizeof(__##qtc_local_buff##name) / sizeof(type)                 \
    };                                                                  \
    QTC_RESIZE_LOCAL_BUFF(name, size)

/**
 * \brief Resize a local buffer defined with QTC_DEF_LOCAL_BUFF for holding
 * .      more elements.
 * \param name name of the buffer
 * \param size new minimum size of the array.
 *
 * This macro resizes a buffer \param name defined with QTC_DEF_LOCAL_BUFF for
 * holding at least \param size elements.
 * \sa QTC_DEF_LOCAL_BUFF
 * \sa QTC_FREE_LOCAL_BUFF
 */
#define QTC_RESIZE_LOCAL_BUFF(name, size) do {                          \
        size_t __new_size = (size);                                     \
        if (__new_size <= (name).l || __new_size <= (name).static_l)    \
            break;                                                      \
        (name).l = __new_size;                                          \
        size_t __alloc_size = sizeof(*(name).p) * __new_size;           \
        if ((name).p == (name).static_p) {                              \
            (name)._p = malloc(__alloc_size);                           \
        } else {                                                        \
            (name)._p = realloc((name)._p, __alloc_size);               \
        }                                                               \
    } while (0)

/**
 * \brief Free a local buffer defined with QTC_DEF_LOCAL_BUFF if necessary.
 * \param name name of the buffer
 *
 * \sa QTC_DEF_LOCAL_BUFF
 * \sa QTC_RESIZE_LOCAL_BUFF
 */
#define QTC_FREE_LOCAL_BUFF(name) do {          \
        if ((name).p != (name).static_p) {      \
            free((name)._p);                    \
        }                                       \
    } while (0)

QTC_BEGIN_DECLS

/**
 * Binary searches an sorted array of \param nmemb objects.
 * \param key The key for the search.
 * \param base pointer to the first element.
 * \param nmemb number of object in the array.
 * \param size size of each object.
 * \param compar function to compare key with an object.
 *
 * \return if a matching object is found, the pointer to the object will be
 *         returned, otherwise, a pointer to where the new object should be
 *         inserted will be returned. If the key is greater than all the
 *         elements in the array, the returned pointer will point to the end
 *         of the list, i.e. out of the bound of the list.
 */
void *qtcBSearch(const void *key, const void *base, size_t nmemb, size_t size,
                 int (*compar)(const void*, const void*));

typedef struct {
    const char *key;
    int val;
} QtcStrMapItem;

typedef struct {
    unsigned num;
    QtcStrMapItem *items;
    bool auto_val;
    bool inited;
    bool case_sensitive;
} QtcStrMap;

void qtcStrMapInit(QtcStrMap *map);
#define QTC_DECL_STR_MAP(map, case_sense, items...)      \
    static QtcStrMapItem __##map##_items[] = {items};    \
    static QtcStrMap map = {                             \
        sizeof(__##map##_items) / sizeof(QtcStrMapItem), \
        __##map##_items,                                 \
        false,                                           \
        false,                                           \
        case_sense,                                      \
    };                                                   \
    if (!map.inited) {                                   \
        qtcStrMapInit(&map);                             \
    }

static inline void
qtcStrMapInitKeys(QtcStrMap *map, const char **keys)
{
    if (map->inited) {
        return;
    }
    for (unsigned i = 0;i < map->num;i++) {
        map->items[i].key = keys[i];
    }
    qtcStrMapInit(map);
}

#define QTC_DECL_STR_MAP_AUTO(map, case_sense, keys...)           \
    static const char *__##map##_keys[] = {keys};                 \
    static QtcStrMapItem __##map##_items[sizeof(__##map##_keys) / \
                                         sizeof(const char*)];    \
    static QtcStrMap map = {                                      \
        sizeof(__##map##_keys) / sizeof(const char*),             \
        __##map##_items,                                          \
        true,                                                     \
        false,                                                    \
        case_sense,                                               \
    };                                                            \
    qtcStrMapInitKeys(&map, __##map##_keys);

int qtcStrMapSearch(const QtcStrMap *map, const char *key, int def);
#define _qtcStrMapSearch(map, key, def, ...)    \
    (qtcStrMapSearch)(map, key, def)
#define qtcStrMapSearch(map, key, def...)       \
    _qtcStrMapSearch(map, key, ##def, 0)

const char *qtcGetProgName();

QTC_END_DECLS

#ifdef __cplusplus
#include <utility>
template<typename T>
QTC_ALWAYS_INLINE static inline bool
qtcOneOf(T &&value)
{
    QTC_UNUSED(value);
    return false;
}
template<typename T, typename First>
QTC_ALWAYS_INLINE static inline bool
qtcOneOf(T &&value, First &&first)
{
    return value == first;
}
template<typename T, typename First, typename... Rest>
QTC_ALWAYS_INLINE static inline bool
qtcOneOf(T &&value, First &&first, Rest &&...rest...)
{
    return value == first || qtcOneOf(std::forward<T>(value),
                                      std::forward<Rest>(rest)...);
}
#define qtcDefault(val, def)                    \
    (([&]() {                                   \
            decltype(val) __val = (val);        \
            return __val ? __val : (def);       \
        })())
#define qtcCall(func, args...)                                          \
    (([&]() {                                                           \
            decltype(func) __func = (func);                             \
            return __func ? __func(args) : decltype(__func(args))();    \
        })())
#else
#define qtcOneOf(exp, args...)                                  \
    ({                                                          \
        const typeof((exp)) __val = (exp);                      \
        const typeof((exp)) __args[] = {args};                  \
        size_t __arg_n = sizeof(__args) / sizeof(__args[0]);    \
        bool __res = false;                                     \
        for (size_t __i = 0;__i < __arg_n;__i++) {              \
            if (__val == __args[__i]) {                         \
                __res = true;                                   \
                break;                                          \
            }                                                   \
        }                                                       \
        __res;                                                  \
    })
#define qtcDefault(val, def)                    \
    ({                                          \
        typeof(val) __val = (val);              \
        __val ? __val : (def);                  \
    })
#define qtcCall(func, args...)                                  \
    ({                                                          \
        typeof(func) __func = (func);                           \
        __func ? __func(args) : (typeof(__func(args)))0;        \
    })
#endif
#define qtcNoneOf(args...) (!qtcOneOf(args))

#endif
