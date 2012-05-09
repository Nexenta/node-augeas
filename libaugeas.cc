/*
 * Copyright (C) 2012, Nexenta Systems, Inc.
 *
 * The contents of this file are subject to the terms of
 * the Common Development and Distribution License ("CDDL").
 * You may not use this file except in compliance with this license.
 *
 * You can obtain a copy of the License at
 * http://www.opensource.org/licenses/CDDL-1.0
 */

#include <string>

#define BUILDING_NODE_EXTENSION

// node.h includes v8.h
#include <node.h>

extern "C" { // Yes, that bad
#include <augeas.h>
}

using namespace v8;


inline std::string aug_error_msg(augeas *aug)
{
    std::string msg = aug_error_message(aug);
    const char *minor = aug_error_minor_message(aug);
    const char *details = aug_error_details(aug);
    if (NULL != minor) {
        msg += " - ";
        msg += minor;
    }
    if (NULL != details) {
        msg += ": ";
        msg += details;
    }
    return msg;
}

inline void throw_aug_error_msg(augeas *aug)
{
    if (AUG_NOERROR != aug_error(aug)) {
        ThrowException(Exception::Error(String::New(aug_error_msg(aug).c_str())));
    } else {
        ThrowException(Exception::Error(
            String::New(
                "An error has occured from Augeas API call, but no description available")));
    }
}


/*
 * Helper function.
 * Converts value of object member *key into std::string.
 * Returns empty string if memder does not exist.
 */
inline std::string memberToString(Handle<Object> obj, const char *key)
{
    Local<Value> m = obj->Get(Local<String>(String::New(key)));
    if (!m->IsUndefined()) {
        String::Utf8Value str(m);
        return std::string(*str);
    } else {
        return std::string();
    }
}

/*
 * Helper function.
 * Converts value of object member *key into uint32.
 * Returns 0 if memder does not exist.
 */
inline uint32_t memberToUint32(Handle<Object> obj, const char *key)
{
    Local<Value> m = obj->Get(Local<String>(String::New(key)));
    if (!m->IsUndefined()) {
        return m->Uint32Value();
    } else {
        return 0;
    }
}

class LibAugeas : public node::ObjectWrap {
public:
    static void Init(Handle<Object> target);
    static Local<Object> New(augeas *aug);
    static Handle<Value> NewInstance(const Arguments& args);

protected:
    augeas * m_aug;
    LibAugeas();
    ~LibAugeas();

    static Persistent<FunctionTemplate> augeasTemplate;
    static Persistent<Function> constructor;

    static Handle<Value> New(const Arguments& args);

    static Handle<Value> defvar     (const Arguments& args);
    static Handle<Value> defnode    (const Arguments& args);
    static Handle<Value> get        (const Arguments& args);
    static Handle<Value> set        (const Arguments& args);
    static Handle<Value> setm       (const Arguments& args);
    static Handle<Value> rm         (const Arguments& args);
    static Handle<Value> mv         (const Arguments& args);
    static Handle<Value> save       (const Arguments& args);
    static Handle<Value> nmatch     (const Arguments& args);
    static Handle<Value> match      (const Arguments& args);
    static Handle<Value> load       (const Arguments& args);
    static Handle<Value> insertAfter  (const Arguments& args);
    static Handle<Value> insertBefore (const Arguments& args);
    static Handle<Value> error      (const Arguments& args);
    static Handle<Value> errorMsg   (const Arguments& args);
};

Persistent<FunctionTemplate> LibAugeas::augeasTemplate;
Persistent<Function> LibAugeas::constructor;

void LibAugeas::Init(Handle<Object> target)
{
    // flags for aug_init():
    NODE_DEFINE_CONSTANT(target, AUG_NONE);
    NODE_DEFINE_CONSTANT(target, AUG_SAVE_BACKUP);
    NODE_DEFINE_CONSTANT(target, AUG_SAVE_NEWFILE);
    NODE_DEFINE_CONSTANT(target, AUG_TYPE_CHECK);
    NODE_DEFINE_CONSTANT(target, AUG_NO_STDINC);
    NODE_DEFINE_CONSTANT(target, AUG_SAVE_NOOP);
    NODE_DEFINE_CONSTANT(target, AUG_NO_LOAD);
    NODE_DEFINE_CONSTANT(target, AUG_NO_MODL_AUTOLOAD);
    NODE_DEFINE_CONSTANT(target, AUG_ENABLE_SPAN);
    NODE_DEFINE_CONSTANT(target, AUG_NO_ERR_CLOSE);

    // error codes:
    NODE_DEFINE_CONSTANT(target, AUG_NOERROR);
    NODE_DEFINE_CONSTANT(target, AUG_ENOMEM);
    NODE_DEFINE_CONSTANT(target, AUG_EINTERNAL);
    NODE_DEFINE_CONSTANT(target, AUG_EPATHX);
    NODE_DEFINE_CONSTANT(target, AUG_ENOMATCH);
    NODE_DEFINE_CONSTANT(target, AUG_EMMATCH);
    NODE_DEFINE_CONSTANT(target, AUG_ESYNTAX);
    NODE_DEFINE_CONSTANT(target, AUG_ENOLENS);
    NODE_DEFINE_CONSTANT(target, AUG_EMXFM);
    NODE_DEFINE_CONSTANT(target, AUG_ENOSPAN);
    NODE_DEFINE_CONSTANT(target, AUG_EMVDESC);
    NODE_DEFINE_CONSTANT(target, AUG_ECMDRUN);
    NODE_DEFINE_CONSTANT(target, AUG_EBADARG);

    augeasTemplate = Persistent<FunctionTemplate>::New(FunctionTemplate::New(New));
    augeasTemplate->SetClassName(String::NewSymbol("Augeas"));
    augeasTemplate->InstanceTemplate()->SetInternalFieldCount(1);

// I do not want copy-n-paste errors here:
#define _NEW_METHOD(m) NODE_SET_PROTOTYPE_METHOD(augeasTemplate, #m, m)
    _NEW_METHOD(defvar);
    _NEW_METHOD(defnode);
    _NEW_METHOD(get);
    _NEW_METHOD(set);
    _NEW_METHOD(setm);
    _NEW_METHOD(rm);
    _NEW_METHOD(mv);
    _NEW_METHOD(save);
    _NEW_METHOD(nmatch);
    _NEW_METHOD(match);
    _NEW_METHOD(load);
    _NEW_METHOD(insertAfter);
    _NEW_METHOD(insertBefore);
    _NEW_METHOD(error);
    _NEW_METHOD(errorMsg);


    constructor = Persistent<Function>::New(augeasTemplate->GetFunction());

    target->Set(String::NewSymbol("Augeas"), constructor);
}

/*
 * Creates an JS object to pass it in callback function.
 * This JS objects wraps an LibAugeas object.
 * Only for using within this C++ code.
 */
Local<Object> LibAugeas::New(augeas *aug)
{
    LibAugeas *obj = new LibAugeas();
    obj->m_aug = aug;
    Local<Object> O = augeasTemplate->InstanceTemplate()->NewInstance();
    obj->Wrap(O);
    return O;
}


/*
 * Creates an JS object from this C++ code,
 * passing arguments received from JS code to
 * the constructor LibAugeas::New(const Arguments& args)
 */
Handle<Value> LibAugeas::NewInstance(const Arguments& args)
{
    HandleScope scope;
    Handle<Value> *argv = NULL;

    int argc = args.Length();
    if (argc > 0) {
        argv = new Handle<Value>[argc];
        assert(argv != NULL);
        for (int i = 0; i < argc; ++i) {
            argv[i] = args[i];
        }
    }

    Local<Object> instance = constructor->NewInstance(argc, argv);

    if (NULL != argv)
        delete [] argv;

    return scope.Close(instance);
}

/*
 * This function is used as a constructor from JS side via
 * var aug = new lib.Augeas(...);
 */
Handle<Value> LibAugeas::New(const Arguments& args)
{
    HandleScope scope;

    LibAugeas *obj = new LibAugeas();

    std::string root;
    std::string loadpath;
    unsigned int flags = 0;

    // Allow passing options in object:
    if (args[0]->IsObject()) {
        Local<Object> obj = args[0]->ToObject();
        root     = memberToString(obj, "root");
        loadpath = memberToString(obj, "loadpath");
        flags    = memberToUint32(obj, "flags");
    } else {
        // C-like way:
        if (args[0]->IsString()) {
            String::Utf8Value p_str(args[0]);
            root = *p_str;
        }
        if (args[1]->IsString()) {
            String::Utf8Value l_str(args[1]);
            loadpath = *l_str;
        }
        if (args[2]->IsNumber()) {
            flags = args[2]->Uint32Value();
        }
    }
    
    obj->m_aug = aug_init(root.c_str(), loadpath.c_str(), flags | AUG_NO_ERR_CLOSE);

    if (NULL == obj->m_aug) { // should not happen
        ThrowException(Exception::Error(String::New("aug_init() badly failed")));
        return scope.Close(Undefined());
    } else if (AUG_NOERROR != aug_error(obj->m_aug)) {
        throw_aug_error_msg(obj->m_aug);
        aug_close(obj->m_aug);
        return scope.Close(Undefined());
    }

    obj->Wrap(args.This());

    return args.This();
}

/*
 * Wrapper of aug_defvar() - define a variable
 * The second argument is optional and if ommited,
 * variable will be removed if exists.
 *
 * On error, throws an exception and returns undefined;
 * on success, returns 0 if expr evaluates to anything
 * other than a nodeset, and the number of nodes if expr
 * evaluates to a nodeset
 */
Handle<Value> LibAugeas::defvar(const Arguments& args)
{
    HandleScope scope;

    if (args.Length() < 1 || args.Length() > 2) {
        ThrowException(Exception::TypeError(String::New("Wrong number of arguments")));
        return scope.Close(Undefined());
    }

    LibAugeas *obj = ObjectWrap::Unwrap<LibAugeas>(args.This());
    String::Utf8Value n_str(args[0]);
    String::Utf8Value e_str(args[1]);

    const char *name = *n_str;
    const char *expr = *e_str;

    /* Returns -1 on error; on success, returns 0 if EXPR evaluates to anything
     * other than a nodeset, and the number of nodes if EXPR evaluates to a nodeset
     */
    int rc = aug_defvar(obj->m_aug, name, args[1]->IsUndefined() ? NULL : expr);
    if (-1 == rc) {
        throw_aug_error_msg(obj->m_aug);
        return scope.Close(Undefined());
    } else {
        return scope.Close(Number::New(rc));
    }
}

/*
 * Wrapper of aug_defnode() - define a node.
 * The defnode command is very useful when you add a node
 * that you need to modify further, e. g. by adding children to it.
 *
 * On error, throws an exception and returns undefined;
 * on success, returns the number of nodes in the nodeset (>= 0)
 * 
 * Arguments:
 * name - required
 * expr - required
 * value - optional
 * callback - optional
 *
 * The last argument could be a function. It will be called (synchronously)
 * with one argument set to True if a node was created, and False if it already existed. 
 */
Handle<Value> LibAugeas::defnode(const Arguments& args)
{
    HandleScope scope;

    if (args.Length() < 2 || args.Length() > 4) {
        ThrowException(Exception::TypeError(String::New("Wrong number of arguments")));
        return scope.Close(Undefined());
    }

    LibAugeas *obj = ObjectWrap::Unwrap<LibAugeas>(args.This());
    String::Utf8Value n_str(args[0]);
    String::Utf8Value e_str(args[1]);
    String::Utf8Value v_str(args[2]);

    const char *name = *n_str;
    const char *expr = *e_str;
    const char *value = ""; // can't be NULL: aug_get will cause segfault
    if (!args[2]->IsUndefined() && !args[2]->IsFunction()) {
        value = *v_str;
    }
    int created;

    /* Returns -1 on error; on success, returns
     * the number of nodes in the nodeset, set created=1 if node created,
     * set created=0 if node already existed.
     */
    int rc = aug_defnode(obj->m_aug, name, expr, value, &created);
    if (-1 == rc) {
        throw_aug_error_msg(obj->m_aug);
        return scope.Close(Undefined());
    } else {
        int last = args.Length() - 1;
        if (args[last]->IsFunction()) {
            Local<Function> cb =  Local<Function>::Cast(args[last]);

            Local<Value> argv[] = {Local<Boolean>::New(Boolean::New(created == 1))};

            TryCatch try_catch;
            cb->Call(Context::GetCurrent()->Global(), 1, argv);
            if (try_catch.HasCaught()) {
                node::FatalException(try_catch);
            }
        }
        return scope.Close(Number::New(rc));
    }
}

/*
 * Wrapper of aug_get() - get exactly one value
 */
Handle<Value> LibAugeas::get(const Arguments& args)
{
    HandleScope scope;

    if (args.Length() != 1) {
        ThrowException(Exception::TypeError(String::New("Wrong number of arguments")));
        return scope.Close(Undefined());
    }

    LibAugeas *obj = ObjectWrap::Unwrap<LibAugeas>(args.This());
    String::Utf8Value p_str(args[0]);

    const char *path = *p_str; // operator*() returns C-string
    const char *value;

    /*
     * Return 1 if there is exactly one node matching PATH,
     * 0 if there is none, and a negative value
     * if there is more than one node matching PATH,
     * or if PATH is not a legal path expression.
     */
    /*
     * The string *value must not be freed by the caller,
     * and is valid as long as its node remains unchanged.
     */
    int rc = aug_get(obj->m_aug, path, &value);
    if (1 == rc) {
      if (value) {
        return scope.Close(String::New(value));
      } else {
        return scope.Close(Null());
      }
    } else if (0 == rc) {
        return scope.Close(Undefined());
    } else if (rc < 0) {
        throw_aug_error_msg(obj->m_aug);
        return scope.Close(Undefined());
    } else {
        ThrowException(Exception::Error(String::New("Unexpected return value of aug_get()")));
        return scope.Close(Undefined());
    }
}

/*
 * Wrapper of aug_set() - set exactly one value
 * Note: this method (as aug_set) does not write any files,
 *       it just changes internal tree. To write files use LibAugeas::save()
 */
Handle<Value> LibAugeas::set(const Arguments& args)
{
    HandleScope scope;

    if (args.Length() != 2) {
        ThrowException(Exception::TypeError(String::New("Wrong number of arguments")));
        return scope.Close(Undefined());
    }

    LibAugeas *obj = ObjectWrap::Unwrap<LibAugeas>(args.This());
    String::Utf8Value p_str(args[0]);
    String::Utf8Value v_str(args[1]);

    const char *path  = *p_str;
    const char *value = *v_str;

    /*
     * 0 on success, -1 on error. It is an error
     * if more than one node matches path.
     */
    int rc = aug_set(obj->m_aug, path, value);
    if (AUG_NOERROR != rc) {
        throw_aug_error_msg(obj->m_aug);
    }
    return scope.Close(Undefined());
}

/*
 * Wrapper of aug_setm() - set the value of multiple nodes in one operation
 * Returns the number of modified nodes on success.
 */
Handle<Value> LibAugeas::setm(const Arguments& args)
{
    HandleScope scope;

    if (args.Length() != 3) {
        ThrowException(Exception::TypeError(String::New("Wrong number of arguments")));
        return scope.Close(Undefined());
    }

    LibAugeas *obj = ObjectWrap::Unwrap<LibAugeas>(args.This());
    String::Utf8Value b_str(args[0]);
    String::Utf8Value s_str(args[1]);
    String::Utf8Value v_str(args[2]);

    const char *base  = *b_str;
    const char *sub   = *s_str;
    const char *value = *v_str;


    int rc = aug_setm(obj->m_aug, base, sub, value);
    if (rc >= 0) {
        return scope.Close(Int32::New(rc));
    } else {
        throw_aug_error_msg(obj->m_aug);
        return scope.Close(Undefined());
    }
}


/*
 * Wrapper of aug_rm() - remove nodes
 * Remove path and all its children. Returns the number of entries removed.
 * All nodes that match PATH, and their descendants, are removed.
 */
Handle<Value> LibAugeas::rm(const Arguments& args)
{
    HandleScope scope;

    if (args.Length() != 1) {
        ThrowException(Exception::TypeError(String::New("Wrong number of arguments")));
        return scope.Close(Undefined());
    }

    LibAugeas *obj = ObjectWrap::Unwrap<LibAugeas>(args.This());
    String::Utf8Value p_str(args[0]);

    const char *path  = *p_str;

    int rc = aug_rm(obj->m_aug, path);
    if (rc >= 0) {
        return scope.Close(Number::New(rc));
    } else {
        throw_aug_error_msg(obj->m_aug);
        return scope.Close(Undefined());
    }

}

/*
 * Wrapper of aug_mv() - move nodes
 */
Handle<Value> LibAugeas::mv(const Arguments& args)
{
    HandleScope scope;

    if (args.Length() != 2) {
        ThrowException(Exception::TypeError(String::New("Wrong number of arguments")));
        return scope.Close(Undefined());
    }

    LibAugeas *obj = ObjectWrap::Unwrap<LibAugeas>(args.This());
    String::Utf8Value src(args[0]);
    String::Utf8Value dst(args[1]);

    const char *source = *src;
    const char *dest   = *dst;

    /*
     * 0 on success, -1 on error. It is an error
     * if more than one node matches path.
     */
    int rc = aug_mv(obj->m_aug, source, dest);
    if (AUG_NOERROR != rc) {
        throw_aug_error_msg(obj->m_aug);
    }
    return scope.Close(Undefined());
}

/*
 * Wrapper of aug_insert(aug, path, label, 0) - insert 'label' after 'path'
 */
Handle<Value> LibAugeas::insertAfter(const Arguments& args)
{
    HandleScope scope;

    if (args.Length() != 2) {
        ThrowException(Exception::TypeError(String::New("Wrong number of arguments")));
        return scope.Close(Undefined());
    }

    LibAugeas *obj = ObjectWrap::Unwrap<LibAugeas>(args.This());
    String::Utf8Value p_str(args[0]);
    String::Utf8Value l_str(args[1]);

    const char *path  = *p_str;
    const char *label = *l_str;

    int rc = aug_insert(obj->m_aug, path, label, 0);
    if (AUG_NOERROR != rc) {
        throw_aug_error_msg(obj->m_aug);
    }
    return scope.Close(Undefined());
}

/*
 * Wrapper of aug_insert(aug, path, label, 1) - insert 'label' before 'path'
 */
Handle<Value> LibAugeas::insertBefore(const Arguments& args)
{
    HandleScope scope;

    if (args.Length() != 2) {
        ThrowException(Exception::TypeError(String::New("Wrong number of arguments")));
        return scope.Close(Undefined());
    }

    LibAugeas *obj = ObjectWrap::Unwrap<LibAugeas>(args.This());
    String::Utf8Value p_str(args[0]);
    String::Utf8Value l_str(args[1]);

    const char *path  = *p_str;
    const char *label = *l_str;

    int rc = aug_insert(obj->m_aug, path, label, 1);
    if (AUG_NOERROR != rc) {
        throw_aug_error_msg(obj->m_aug);
    }
    return scope.Close(Undefined());
}

/*
 * Wrapper of aug_error()
 * Returns the error code from the last API call
 */
Handle<Value> LibAugeas::error(const Arguments& args)
{
    HandleScope scope;

    if (args.Length() != 0) {
        ThrowException(Exception::TypeError(String::New("Function does not accept arguments")));
        return scope.Close(Undefined());
    }

    LibAugeas *obj = ObjectWrap::Unwrap<LibAugeas>(args.This());

    int rc = aug_error(obj->m_aug);
    return scope.Close(Int32::New(rc));
}

/*
 * Returns the error message from the last API call,
 * including all details.
 */
Handle<Value> LibAugeas::errorMsg(const Arguments& args)
{
    HandleScope scope;

    if (args.Length() != 0) {
        ThrowException(Exception::TypeError(String::New("Function does not accept arguments")));
        return scope.Close(Undefined());
    }

    LibAugeas *obj = ObjectWrap::Unwrap<LibAugeas>(args.This());

    return scope.Close(String::New(aug_error_msg(obj->m_aug).c_str()));
}



struct SaveUV {
    uv_work_t request;
    Persistent<Function> callback;
    augeas *aug;
    int rc; // = aug_save(), 0 on success, -1 on error
};

void saveWork(uv_work_t *req)
{
    SaveUV *suv = static_cast<SaveUV*>(req->data);
    suv->rc = aug_save(suv->aug);
}

/*
 * Execute JS callback after saveWork() terminated
 */
void saveAfter(uv_work_t* req)
{
    HandleScope scope;

    SaveUV *suv = static_cast<SaveUV*>(req->data);
    Local<Value> argv[] = {Int32::New(suv->rc)};

    TryCatch try_catch;
    suv->callback->Call(Context::GetCurrent()->Global(), 1, argv);
    if (try_catch.HasCaught()) {
        node::FatalException(try_catch);
    }

    suv->callback.Dispose();
    delete suv;
}

/*
 * Wrapper of aug_save() - save changed files.
 *
 * Without arguments this function performs blocking saving
 * and throws an exception on error.
 *
 * The only argument allowed is a callback function.
 * If such an argument is given this function performs
 * non-blocking (async) saving, and after saving is done (or failed)
 * executes the callback with one integer argument - return value of aug_save(),
 * i. e. 0 on success, -1 on failure.
 *
 * NOTE: multiple async calls of this function (from the same augeas object)
 * will result in crash (double free or segfault) because of using
 * shared augeas handle.
 *
 * Always returns undefined.
 */
Handle<Value> LibAugeas::save(const Arguments& args)
{
    HandleScope scope;

    LibAugeas *obj = ObjectWrap::Unwrap<LibAugeas>(args.This());

    // if no args, save files synchronously (blocking):
    if (args.Length() == 0) {
        int rc = aug_save(obj->m_aug);
        if (AUG_NOERROR != rc) {
            ThrowException(Exception::Error(String::New("Failed to write files")));
        }
        // single argument is a function - async:
    } else if ((args.Length() == 1) && args[0]->IsFunction()) {
        SaveUV * suv = new SaveUV();
        suv->request.data = suv;
        suv->aug = obj->m_aug;
        suv->callback = Persistent<Function>::New(
                            Local<Function>::Cast(args[0]));
        uv_queue_work(uv_default_loop(), &suv->request, saveWork, saveAfter);
    } else {
        ThrowException(Exception::Error(String::New("Callback function or nothing")));
    }

    return scope.Close(Undefined());
}

/*
 * Wrapper of aug_match(aug, path, NULL) - count all nodes matching path expression
 * Returns the number of found nodes.
 * Note: aug_match() allocates memory if the third argument is not NULL,
 * in this function we always set it to NULL and get only number of found nodes.
 */
Handle<Value> LibAugeas::nmatch(const Arguments& args)
{
    HandleScope scope;

    if (args.Length() != 1) {
        ThrowException(Exception::TypeError(String::New("Wrong number of arguments")));
        return scope.Close(Undefined());
    }

    LibAugeas *obj = ObjectWrap::Unwrap<LibAugeas>(args.This());
    String::Utf8Value p_str(args[0]);

    const char *path  = *p_str;

    int rc = aug_match(obj->m_aug, path, NULL);
    if (rc >= 0) {
        return scope.Close(Number::New(rc));
    } else {
        throw_aug_error_msg(obj->m_aug);
        return scope.Close(Undefined());
    }
}

/*
 * Wrapper of aug_match(, , non-NULL).
 * Returns an array of nodes matching given path expression
 */
Handle<Value> LibAugeas::match(const Arguments& args)
{
    HandleScope scope;

    if (args.Length() != 1) {
        ThrowException(Exception::TypeError(String::New("Wrong number of arguments")));
        return scope.Close(Undefined());
    }

    LibAugeas *obj = ObjectWrap::Unwrap<LibAugeas>(args.This());
    String::Utf8Value p_str(args[0]);

    const char *path  = *p_str;
    char **matches = NULL;

    int rc = aug_match(obj->m_aug, path, &matches);
    if (rc >= 0) {
        Local<Array> result = Array::New(rc);
        if (NULL != matches) {
            for (int i = 0; i < rc; ++i) {
                result->Set(Number::New(i), String::New(matches[i]));
                free(matches[i]);
            }
            free(matches);
        }
        return scope.Close(result);
    } else {
        throw_aug_error_msg(obj->m_aug);
        return scope.Close(Undefined());
    }
}

/*
 * Wrapper of aug_load() - load /files
 */
Handle<Value> LibAugeas::load(const Arguments& args)
{
    HandleScope scope;

    if (args.Length() != 0) {
        ThrowException(Exception::TypeError(String::New("Function does not accept arguments")));
        return scope.Close(Undefined());
    }

    LibAugeas *obj = ObjectWrap::Unwrap<LibAugeas>(args.This());

    /* aug_load() returns -1 on error, 0 on success. Success includes the case
     * where some files could not be loaded. Details of such files can be found
     * as '/augeas//error'.
     */
    int rc = aug_load(obj->m_aug);
    if (AUG_NOERROR != rc) {
        ThrowException(Exception::Error(String::New("Failed to load files")));
    }

    return scope.Close(Undefined());
}


LibAugeas::LibAugeas() : m_aug(NULL)
{
}

LibAugeas::~LibAugeas()
{
    aug_close(m_aug);
}


struct CreateAugeasUV {
    uv_work_t request;
    Persistent<Function> callback;
    std::string root;
    std::string loadpath;
    std::string lens;
    std::string incl;
    std::string excl;
    unsigned int flags;
    augeas *aug;
};


/*
 * This function should immediately return if any call to augeas API fails.
 * The caller should check aug_error() before doing anything.
 */
void createAugeasWork(uv_work_t *req)
{
    int rc = AUG_NOERROR;

    CreateAugeasUV *her = static_cast<CreateAugeasUV*>(req->data);

    unsigned int flags;

    // Always set AUG_NO_ERR_CLOSE:
    flags = AUG_NO_ERR_CLOSE | her->flags;

    // do not load all lenses if a specific lens is given,
    // ignore any setting in flags.
    // XXX: AUG_NO_MODL_AUTOLOAD implies AUG_NO_LOAD
    if (!her->lens.empty()) {
        flags |= AUG_NO_MODL_AUTOLOAD;
    } else {
        flags &= ~AUG_NO_MODL_AUTOLOAD;
    }

    her->aug = aug_init(her->root.c_str(), her->loadpath.c_str(), flags);
    rc = aug_error(her->aug);
    if (AUG_NOERROR != rc)
        return;

    if (!her->lens.empty()) {
        // specifying which lens to load
        // /augeas/load/<random-name>/lens = e. g.: "hosts.lns" or "@Hosts_Access"
        std::string basePath = "/augeas/load/1"; // "1" is a random valid name :-)
        std::string lensPath = basePath + "/lens";
        std::string lensVal  = her->lens;
        if ((lensVal[0] != '@') // if not a module
                && (lensVal.rfind(".lns") == std::string::npos))
        {
            lensVal += ".lns";
        }
        rc = aug_set(her->aug, lensPath.c_str(), lensVal.c_str());
        if (AUG_NOERROR != rc)
            return;

        if (!her->incl.empty()) {
            // specifying which files to load :
            // /augeas/load/<random-name>/incl = glob
            std::string inclPath = basePath + "/incl";
            rc = aug_set(her->aug, inclPath.c_str(), her->incl.c_str());
            if (AUG_NOERROR != rc)
                return;
        }
        if (!her->excl.empty()) {
            // specifying which files NOT to load
            // /augeas/load/<random-name>/excl = glob (e. g. "*.dpkg-new")
            std::string exclPath = basePath + "/excl";
            rc = aug_set(her->aug, exclPath.c_str(), her->excl.c_str());
            if (AUG_NOERROR != rc)
                return;
        }

        rc = aug_load(her->aug);
        if (AUG_NOERROR != rc)
            return;
    }
}

void createAugeasAfter(uv_work_t* req)
{
    HandleScope scope;

    CreateAugeasUV *her = static_cast<CreateAugeasUV*>(req->data);
    Local<Value> argv[] = {LibAugeas::New(her->aug)};

    TryCatch try_catch;
    her->callback->Call(Context::GetCurrent()->Global(), 1, argv);
    if (try_catch.HasCaught()) {
        node::FatalException(try_catch);
    }

    her->callback.Dispose();
    delete her;
}

/*
 * Creates augeas object from JS side either in sync or async way
 * depending on the last argument:
 *
 * lib.createAugeas([...], function(aug) {...}) - async
 *
 * or:
 *
 * var aug = lib.createAugeas([...]) - sync
 *
 * The latter is equivalent to var aug = new lib.Augeas([...]);
 */
Handle<Value> createAugeas(const Arguments& args)
{
    HandleScope scope;

    /*
     * If the last argument is a function, create augeas
     * in an async way, and then pass it to that function.
     */
    bool async = (args.Length() > 0) && (args[args.Length()-1]->IsFunction());

    if (async) {
        CreateAugeasUV *her = new CreateAugeasUV();
        her->request.data = her;
        her->callback = Persistent<Function>::New(
                            Local<Function>::Cast(args[args.Length()-1]));

        /* TODO:
         * Use more then one object.
         * From JS point it might look like this:
         * createAugeas({root:..., loadpath:..., lens:...}, {lens:...}, {lens:...}, ..., callback);
         * In the first object there might be root, loadpath or flags members, and lenses
         * might be omitted. In other objects only lens, incl and excl members should be allowed.
         * ** flags are filterred in createAugeasWork() **
         */
        if (args[0]->IsObject()) {
            Local<Object> obj = args[0]->ToObject();
            her->root = memberToString(obj, "root");
            her->loadpath = memberToString(obj, "loadpath");
            her->flags = memberToUint32(obj, "flags");
            her->lens = memberToString(obj, "lens");
            her->incl = memberToString(obj, "incl");
            her->excl = memberToString(obj, "excl");
        }

        uv_queue_work(uv_default_loop(), &her->request,
                      createAugeasWork, createAugeasAfter);

        return scope.Close(Undefined());
    } else { // sync
        return scope.Close(LibAugeas::NewInstance(args));
    }
}



void init(Handle<Object> target)
{
    LibAugeas::Init(target);

    target->Set(String::NewSymbol("createAugeas"),
                FunctionTemplate::New(createAugeas)->GetFunction());
}

NODE_MODULE(libaugeas, init)

