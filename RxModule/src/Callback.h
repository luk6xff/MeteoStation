/*
 * Callback.h
 *
 *  Created on: 18-09-2016
 *      Author: igbt6
 */

#ifndef CALLBACK_H_
#define CALLBACK_H_
#include "string.h"

/** Callback class based on template specialization
 *
 * @Note Synchronization level: Not protected
 */
template <typename F>
class Callback;


/** Callback class based on template specialization
 *
 * @Note Synchronization level: Not protected
 */
template <typename R, typename A0>
class Callback<R(A0)> {
public:
    /** Create a Callback with a static function
     *  @param func Static function to attach
     */
    Callback(R (*func)(A0) = 0) {
        attach(func);
    }

    /** Create a Callback with a static function and bound pointer
     *  @param obj  Pointer to object to bind to function
     *  @param func Static function to attach
     */
    template<typename T>
    Callback(T *obj, R (*func)(T*, A0)) {
        attach(obj, func);
    }

    /** Create a Callback with a static function and bound pointer
     *  @param obj  Pointer to object to bind to function
     *  @param func Static function to attach
     */
    template<typename T>
    Callback(const T *obj, R (*func)(const T*, A0)) {
        attach(obj, func);
    }

    /** Create a Callback with a static function and bound pointer
     *  @param obj  Pointer to object to bind to function
     *  @param func Static function to attach
     */
    template<typename T>
    Callback(volatile T *obj, R (*func)(volatile T*, A0)) {
        attach(obj, func);
    }

    /** Create a Callback with a static function and bound pointer
     *  @param obj  Pointer to object to bind to function
     *  @param func Static function to attach
     */
    template<typename T>
    Callback(const volatile T *obj, R (*func)(const volatile T*, A0)) {
        attach(obj, func);
    }

    /** Create a Callback with a member function
     *  @param obj  Pointer to object to invoke member function on
     *  @param func Member function to attach
     */
    template<typename T>
    Callback(T *obj, R (T::*func)(A0)) {
        attach(obj, func);
    }

    /** Create a Callback with a member function
     *  @param obj  Pointer to object to invoke member function on
     *  @param func Member function to attach
     */
    template<typename T>
    Callback(const T *obj, R (T::*func)(A0) const) {
        attach(obj, func);
    }

    /** Create a Callback with a member function
     *  @param obj  Pointer to object to invoke member function on
     *  @param func Member function to attach
     */
    template<typename T>
    Callback(volatile T *obj, R (T::*func)(A0) volatile) {
        attach(obj, func);
    }

    /** Create a Callback with a member function
     *  @param obj  Pointer to object to invoke member function on
     *  @param func Member function to attach
     */
    template<typename T>
    Callback(const volatile T *obj, R (T::*func)(A0) const volatile) {
        attach(obj, func);
    }

    /** Attach a static function
     *  @param func Static function to attach
     */
    void attach(R (*func)(A0)) {
        struct local {
            static R _thunk(void*, const void *func, A0 a0) {
                return (*static_cast<R (*const *)(A0)>(func))(
                        a0);
            }
        };

        memset(&_func, 0, sizeof _func);
        memcpy(&_func, &func, sizeof func);
        _obj = 0;
        _thunk = func ? &local::_thunk : 0;
    }

    /** Attach a Callback
     *  @param func The Callback to attach
     */
    void attach(const Callback<R(A0)> &func) {
        memset(&_func, 0, sizeof _func);
        memcpy(&_func, &func._func, sizeof func);
        _obj = func._obj;
        _thunk = func._thunk;
    }

    /** Attach a static function with a bound pointer
     *  @param obj  Pointer to object to bind to function
     *  @param func Static function to attach
     */
    template <typename T>
    void attach(T *obj, R (*func)(T*, A0)) {
        struct local {
            static R _thunk(void *obj, const void *func, A0 a0) {
                return (*static_cast<R (*const *)(T*, A0)>(func))(
                        (T*)obj, a0);
            }
        };

        memset(&_func, 0, sizeof _func);
        memcpy(&_func, &func, sizeof func);
        _obj = (void*)obj;
        _thunk = &local::_thunk;
    }

    /** Attach a static function with a bound pointer
     *  @param obj  Pointer to object to bind to function
     *  @param func Static function to attach
     */
    template <typename T>
    void attach(const T *obj, R (*func)(const T*, A0)) {
        struct local {
            static R _thunk(void *obj, const void *func, A0 a0) {
                return (*static_cast<R (*const *)(const T*, A0)>(func))(
                        (const T*)obj, a0);
            }
        };

        memset(&_func, 0, sizeof _func);
        memcpy(&_func, &func, sizeof func);
        _obj = (void*)obj;
        _thunk = &local::_thunk;
    }

    /** Attach a static function with a bound pointer
     *  @param obj  Pointer to object to bind to function
     *  @param func Static function to attach
     */
    template <typename T>
    void attach(volatile T *obj, R (*func)(volatile T*, A0)) {
        struct local {
            static R _thunk(void *obj, const void *func, A0 a0) {
                return (*static_cast<R (*const *)(volatile T*, A0)>(func))(
                        (volatile T*)obj, a0);
            }
        };

        memset(&_func, 0, sizeof _func);
        memcpy(&_func, &func, sizeof func);
        _obj = (void*)obj;
        _thunk = &local::_thunk;
    }

    /** Attach a static function with a bound pointer
     *  @param obj  Pointer to object to bind to function
     *  @param func Static function to attach
     */
    template <typename T>
    void attach(const volatile T *obj, R (*func)(const volatile T*, A0)) {
        struct local {
            static R _thunk(void *obj, const void *func, A0 a0) {
                return (*static_cast<R (*const *)(const volatile T*, A0)>(func))(
                        (const volatile T*)obj, a0);
            }
        };

        memset(&_func, 0, sizeof _func);
        memcpy(&_func, &func, sizeof func);
        _obj = (void*)obj;
        _thunk = &local::_thunk;
    }

    /** Attach a member function
     *  @param obj  Pointer to object to invoke member function on
     *  @param func Member function to attach
     */
    template<typename T>
    void attach(T *obj, R (T::*func)(A0)) {
        struct local {
            static R _thunk(void *obj, const void *func, A0 a0) {
                return (((T*)obj)->*
                        (*static_cast<R (T::*const *)(A0)>(func)))(
                        a0);
            }
        };

        memset(&_func, 0, sizeof _func);
        memcpy(&_func, &func, sizeof func);
        _obj = (void*)obj;
        _thunk = &local::_thunk;
    }

    /** Attach a member function
     *  @param obj  Pointer to object to invoke member function on
     *  @param func Member function to attach
     */
    template<typename T>
    void attach(const T *obj, R (T::*func)(A0) const) {
        struct local {
            static R _thunk(void *obj, const void *func, A0 a0) {
                return (((const T*)obj)->*
                        (*static_cast<R (T::*const *)(A0) const>(func)))(
                        a0);
            }
        };

        memset(&_func, 0, sizeof _func);
        memcpy(&_func, &func, sizeof func);
        _obj = (void*)obj;
        _thunk = &local::_thunk;
    }

    /** Attach a member function
     *  @param obj  Pointer to object to invoke member function on
     *  @param func Member function to attach
     */
    template<typename T>
    void attach(volatile T *obj, R (T::*func)(A0) volatile) {
        struct local {
            static R _thunk(void *obj, const void *func, A0 a0) {
                return (((volatile T*)obj)->*
                        (*static_cast<R (T::*const *)(A0) volatile>(func)))(
                        a0);
            }
        };

        memset(&_func, 0, sizeof _func);
        memcpy(&_func, &func, sizeof func);
        _obj = (void*)obj;
        _thunk = &local::_thunk;
    }

    /** Attach a member function
     *  @param obj  Pointer to object to invoke member function on
     *  @param func Member function to attach
     */
    template<typename T>
    void attach(const volatile T *obj, R (T::*func)(A0) const volatile) {
        struct local {
            static R _thunk(void *obj, const void *func, A0 a0) {
                return (((const volatile T*)obj)->*
                        (*static_cast<R (T::*const *)(A0) const volatile>(func)))(
                        a0);
            }
        };

        memset(&_func, 0, sizeof _func);
        memcpy(&_func, &func, sizeof func);
        _obj = (void*)obj;
        _thunk = &local::_thunk;
    }

    /** Call the attached function
     */
    R call(A0 a0) const {
        return _thunk(_obj, &_func, a0);
    }

    /** Call the attached function
     */
    R operator()(A0 a0) const {
        return call(a0);
    }

    /** Test if function has been attached
     */
    operator bool() const {
        return _thunk;
    }

    /** Test for equality
     */
    friend bool operator==(const Callback &l, const Callback &r) {
        return memcmp(&l, &r, sizeof(Callback)) == 0;
    }

    /** Test for inequality
     */
    friend bool operator!=(const Callback &l, const Callback &r) {
        return !(l == r);
    }

    /** Static thunk for passing as C-style function
     *  @param func Callback to call passed as void pointer
     */
    static R thunk(void *func, A0 a0) {
        return static_cast<Callback<R(A0)>*>(func)->call(
                a0);
    }

private:
    // Stored as pointer to function and pointer to optional object
    // Function pointer is stored as union of possible function types
    // to garuntee proper size and alignment
    struct _class;
    union {
        void (*_staticfunc)();
        void (*_boundfunc)(_class *);
        void (_class::*_methodfunc)();
    } _func;

    void *_obj;

    // Thunk registered on attach to dispatch calls
    R (*_thunk)(void*, const void*, A0);
};


// Internally used event type
typedef Callback<void(int)> event_callback_t;


/** Create a callback class with type infered from the arguments
 *
 *  @param func Static function to attach
 *  @return     Callback with infered type
 */
template <typename R, typename A0>
Callback<R(A0)> callback(R (*func)(A0) = 0) {
    return Callback<R(A0)>(func);
}

/** Create a callback class with type infered from the arguments
 *
 *  @param func Static function to attach
 *  @return     Callback with infered type
 */
template <typename R, typename A0>
Callback<R(A0)> callback(const Callback<R(A0)> &func) {
    return Callback<R(A0)>(func);
}

/** Create a callback class with type infered from the arguments
 *
 *  @param obj  Optional pointer to object to bind to function
 *  @param func Static function to attach
 *  @return     Callback with infered type
 */
template <typename T, typename R, typename A0>
Callback<R(A0)> callback(T *obj, R (*func)(T*, A0)) {
    return Callback<R(A0)>(obj, func);
}

/** Create a callback class with type infered from the arguments
 *
 *  @param obj  Optional pointer to object to bind to function
 *  @param func Static function to attach
 *  @return     Callback with infered type
 */
template <typename T, typename R, typename A0>
Callback<R(A0)> callback(const T *obj, R (*func)(const T*, A0)) {
    return Callback<R(A0)>(obj, func);
}

/** Create a callback class with type infered from the arguments
 *
 *  @param obj  Optional pointer to object to bind to function
 *  @param func Static function to attach
 *  @return     Callback with infered type
 */
template <typename T, typename R, typename A0>
Callback<R(A0)> callback(volatile T *obj, R (*func)(volatile T*, A0)) {
    return Callback<R(A0)>(obj, func);
}

/** Create a callback class with type infered from the arguments
 *
 *  @param obj  Optional pointer to object to bind to function
 *  @param func Static function to attach
 *  @return     Callback with infered type
 */
template <typename T, typename R, typename A0>
Callback<R(A0)> callback(const volatile T *obj, R (*func)(const volatile T*, A0)) {
    return Callback<R(A0)>(obj, func);
}

/** Create a callback class with type infered from the arguments
 *
 *  @param obj  Optional pointer to object to bind to function
 *  @param func Static function to attach
 *  @return     Callback with infered type
 */
template<typename T, typename R, typename A0>
Callback<R(A0)> callback(T *obj, R (T::*func)(A0)) {
    return Callback<R(A0)>(obj, func);
}

/** Create a callback class with type infered from the arguments
 *
 *  @param obj  Optional pointer to object to bind to function
 *  @param func Static function to attach
 *  @return     Callback with infered type
 */
template<typename T, typename R, typename A0>
Callback<R(A0)> callback(const T *obj, R (T::*func)(A0) const) {
    return Callback<R(A0)>(obj, func);
}

/** Create a callback class with type infered from the arguments
 *
 *  @param obj  Optional pointer to object to bind to function
 *  @param func Static function to attach
 *  @return     Callback with infered type
 */
template<typename T, typename R, typename A0>
Callback<R(A0)> callback(volatile T *obj, R (T::*func)(A0) volatile) {
    return Callback<R(A0)>(obj, func);
}

/** Create a callback class with type infered from the arguments
 *
 *  @param obj  Optional pointer to object to bind to function
 *  @param func Static function to attach
 *  @return     Callback with infered type
 */
template<typename T, typename R, typename A0>
Callback<R(A0)> callback(const volatile T *obj, R (T::*func)(A0) const volatile) {
    return Callback<R(A0)>(obj, func);
}




#endif /* CALLBACK_H_ */
