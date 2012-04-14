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


#define BUILDING_NODE_EXTENSION

// node.h includes v8.h
#include <node.h>

extern "C" { // Yes, that bad
#include <augeas.h>
}

using namespace v8;

class LibAugeas : public node::ObjectWrap {
public:
    static void Init(Handle<Object> target);

protected:
    augeas * m_aug;
    LibAugeas();
    ~LibAugeas();

    static Handle<Value> New(const Arguments& args);

    static Handle<Value> get    (const Arguments& args);
    static Handle<Value> set    (const Arguments& args);
    static Handle<Value> setm   (const Arguments& args);
    static Handle<Value> rm     (const Arguments& args);
    static Handle<Value> mv     (const Arguments& args);
    static Handle<Value> save   (const Arguments& args);
    static Handle<Value> nmatch (const Arguments& args);
};

void LibAugeas::Init(Handle<Object> target)
{
    Local<FunctionTemplate> tpl = FunctionTemplate::New(New);
    tpl->SetClassName(String::NewSymbol("Augeas"));
    tpl->InstanceTemplate()->SetInternalFieldCount(1);

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

// I do not want copy-n-paste errors here:
#define _NEW_METHOD(m) NODE_SET_PROTOTYPE_METHOD(tpl, #m, m)
    _NEW_METHOD(get);
    _NEW_METHOD(set);
    _NEW_METHOD(setm);
    _NEW_METHOD(rm);
    _NEW_METHOD(mv);
    _NEW_METHOD(save);
    _NEW_METHOD(nmatch);

    //TODO:
    // _NEW_METHOD(insert);
    // _NEW_METHOD(match);

    Persistent<Function> constructor =
        Persistent<Function>::New(tpl->GetFunction());

    target->Set(String::NewSymbol("Augeas"), constructor);
}

Handle<Value> LibAugeas::New(const Arguments& args)
{
    HandleScope scope;

    LibAugeas *obj = new LibAugeas();

    const char *root = NULL;
    const char *loadpath = NULL;
    unsigned int flags = 0;


    if (args[0]->IsString()) {
        String::Utf8Value p_str(args[0]);
        root = *p_str;
    }
    if (args[1]->IsString()) {
        String::Utf8Value l_str(args[1]);
        loadpath = *l_str;
    }
    if (args[2]->IsNumber()) {
        flags = args[2]->Int32Value();
    }

    obj->m_aug = aug_init(root, loadpath, flags);

    if (NULL == obj->m_aug) {
        ThrowException(Exception::Error(String::New("aug_init() failed")));
        return scope.Close(Undefined());
    }

    obj->Wrap(args.This());

    return args.This();
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
        return scope.Close(String::New(value));
    } else if (0 == rc) {
        return scope.Close(Undefined());
    } else if (rc < 0) {
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
    if (0 == rc) {
        return scope.Close(Undefined());
    } else {
        ThrowException(Exception::Error(String::New("aug_set() failed")));
        return scope.Close(Undefined());
    }
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
        return scope.Close(Number::New(rc));
    } else {
        ThrowException(Exception::Error(String::New("aug_setm() failed")));
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
        ThrowException(Exception::Error(String::New("Failed to remove node")));
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
    if (0 == rc) {
        return scope.Close(Undefined());
    } else {
        ThrowException(Exception::Error(String::New("aug_mv() failed")));
        return scope.Close(Undefined());
    }
}
/*
 * Wrapper of aug_save() - save changed files
 * TODO: The exact behavior of aug_save can be
 *       fine-tuned by modifying the node /augeas/save
 *       prior to calling aug_save(). The valid values for this node are:
 *       overwrite, backup, newfile, noop.
 */
Handle<Value> LibAugeas::save(const Arguments& args)
{
    HandleScope scope;

    if (args.Length() != 0) {
        ThrowException(Exception::TypeError(String::New("Wrong number of arguments")));
        return scope.Close(Undefined());
    }

    LibAugeas *obj = ObjectWrap::Unwrap<LibAugeas>(args.This());

    int rc = aug_save(obj->m_aug);
    if (0 == rc) {
        // ok
        return scope.Close(Undefined());
    } else {
        /* TODO: error description is under /augeas/.../error
         * Example:
         * /augeas/files/etc/hosts/error = "open_augnew"
         * /augeas/files/etc/hosts/error/message = "No such file or directory"
         */
        ThrowException(Exception::Error(String::New("Failed to write files")));
        return scope.Close(Undefined());
    }

}

/*
 * Wrapper of aug_match(, , NULL) - count all nodes matching path expression
 * Returns the number of found nodes.
 * Note: aug_match() allocates memory if the third argument is not NULL,
 *       in this function we always set it to NULL and get only number
 *       of found nodes.
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
        ThrowException(Exception::Error(String::New("aug_match() failed")));
        return scope.Close(Undefined());
    }
}


LibAugeas::LibAugeas() : m_aug(NULL)
{
}

LibAugeas::~LibAugeas()
{
    aug_close(m_aug);
}



void init(Handle<Object> target)
{
    LibAugeas::Init(target);
}

NODE_MODULE(libaugeas, init)

