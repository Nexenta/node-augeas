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
#include <map>
#include <libgen.h>
#include <string.h>

#define BUILDING_NODE_EXTENSION

// node.h includes v8.h
#include <node.h>

extern "C" { // Yes, that bad
#include <augeas.h>
}

using namespace v8;


inline void throw_aug_error_msg(augeas *aug)
{
    std::string msg = aug_error_message(aug);
    const char *minor = aug_error_minor_message(aug);
    const char *details = aug_error_details(aug);
    if (NULL != minor) {
        msg += ": ";
        msg += minor;
    }
    if (NULL != details) {
        msg += " -> ";
        msg += details;
    }
    ThrowException(Exception::Error(String::New(msg.c_str())));
}


class LibAugeas : public node::ObjectWrap {
public:
    static void Init(Handle<Object> target);
    static Local<Object> New(augeas *aug);

protected:
    augeas * m_aug;
    LibAugeas();
    ~LibAugeas();

    static Persistent<FunctionTemplate> augeasTemplate;

    static Handle<Value> New(const Arguments& args);

    static Handle<Value> get        (const Arguments& args);
    static Handle<Value> set        (const Arguments& args);
    static Handle<Value> setm       (const Arguments& args);
    static Handle<Value> rm         (const Arguments& args);
    static Handle<Value> mv         (const Arguments& args);
    static Handle<Value> save       (const Arguments& args);
    static Handle<Value> nmatch     (const Arguments& args);
    static Handle<Value> match      (const Arguments& args);
    static Handle<Value> load       (const Arguments& args);
    static Handle<Value> insert     (const Arguments& args);
    static Handle<Value> error      (const Arguments& args);
};

Persistent<FunctionTemplate> LibAugeas::augeasTemplate;

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
    _NEW_METHOD(get);
    _NEW_METHOD(set);
    _NEW_METHOD(setm);
    _NEW_METHOD(rm);
    _NEW_METHOD(mv);
    _NEW_METHOD(save);
    _NEW_METHOD(nmatch);
    _NEW_METHOD(match);
    _NEW_METHOD(load);
    _NEW_METHOD(insert);
    _NEW_METHOD(error);

    //TODO:
    // _NEW_METHOD(match);

    Persistent<Function> constructor =
        Persistent<Function>::New(augeasTemplate->GetFunction());

    target->Set(String::NewSymbol("Augeas"), constructor);
}

/*
 * Returns an JS object to pass it in callback function.
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

Handle<Value> LibAugeas::New(const Arguments& args)
{
    HandleScope scope;

    LibAugeas *obj = new LibAugeas();

    std::string root;
    std::string loadpath;
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

    obj->m_aug = aug_init(root.length() ? root.c_str() : NULL,
                          loadpath.length() ? loadpath.c_str() : NULL, flags);

    /*
     * If flags have AUG_NO_ERR_CLOSE aug_init() might return non-null
     * augeas handle which can be used to get error code and message.
     */
    if (NULL == obj->m_aug) {
        ThrowException(Exception::Error(String::New("aug_init() failed")));
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
    if (0 == rc) {
        return scope.Close(Undefined());
    } else {
        throw_aug_error_msg(obj->m_aug);
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
    if (0 == rc) {
        return scope.Close(Undefined());
    } else {
        throw_aug_error_msg(obj->m_aug);
        return scope.Close(Undefined());
    }
}

/*
 * Wrapper of aug_insert() - insert node
 * The third argument (args[2]) is optional boolean, and is False by default.
 */
Handle<Value> LibAugeas::insert(const Arguments& args)
{
    HandleScope scope;

    if (args.Length() < 2 || args.Length() > 3) {
        ThrowException(Exception::TypeError(String::New("Wrong number of arguments")));
        return scope.Close(Undefined());
    }

    LibAugeas *obj = ObjectWrap::Unwrap<LibAugeas>(args.This());
    String::Utf8Value p_str(args[0]);
    String::Utf8Value l_str(args[1]);

    const char *path  = *p_str;
    const char *label = *l_str;

    /*
     * inserting into the tree just before path if 1,
     * or just after path if 0
     */
    int before = args[2]->ToBoolean()->Value() ? 1 : 0;

    int rc = aug_insert(obj->m_aug, path, label, before);
    if (0 == rc) {
        return scope.Close(Number::New(rc));
    } else {
        throw_aug_error_msg(obj->m_aug);
        return scope.Close(Undefined());
    }
}

/*
 * Wrapper of aug_error()
 * Return the error code from the last API call
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
        assert(matches != NULL);
        Local<Array> result = Array::New(rc);
        for (int i = 0; i < rc; ++i) {
            result->Set(Number::New(i), String::New(matches[i]));
            free(matches[i]);
        }
        free(matches);
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
        ThrowException(Exception::TypeError(String::New("Wrong number of arguments")));
        return scope.Close(Undefined());
    }

    LibAugeas *obj = ObjectWrap::Unwrap<LibAugeas>(args.This());

    int rc = aug_load(obj->m_aug);
    if (0 == rc) {
        // ok
        return scope.Close(Undefined());
    } else {
        /* TODO: error description is under /augeas/.../error
         * Example:
         * /augeas/files/etc/hosts/error = "open_augnew"
         * /augeas/files/etc/hosts/error/message = "No such file or directory"
         */
        ThrowException(Exception::Error(String::New("Failed to load files")));
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


struct HeraclesUV {
    uv_work_t request;
    Persistent<Function> callback;
    std::string lens; // TODO: maybe array
    std::string incl; // TODO: maybe array
    std::string excl; // TODO: maybe array
    augeas *aug; // = aug_init(...)
};


/*
 * This function should immediately return if any call to augeas API fails.
 * The caller should check aug_error() before doing anything.
 */
void heraclesWork(uv_work_t *req)
{
    int rc = AUG_NOERROR;

    HeraclesUV *her = static_cast<HeraclesUV*>(req->data);

    unsigned int flags = AUG_NO_ERR_CLOSE;

    // do not load all lenses if a specific lens is given:
    if (!her->lens.empty()) {
        flags |= AUG_NO_MODL_AUTOLOAD;
        // do not load all files if a specific file is given.
        if (!her->incl.empty()) {
            flags |= AUG_NO_LOAD;
        }
    }

    her->aug = aug_init(NULL, NULL, flags);
    rc = aug_error(her->aug);
    if (AUG_NOERROR != rc)
        return;

    if (!her->lens.empty()) {
        // specifying which lens to load
        // /augeas/load/<random-name>/lens = e. g.: "hosts.lns" or "@Hosts_Access"
        std::string basePath = "/augeas/load/" + her->lens; // any name
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
            std::string exclPath = basePath + "/incl";
            rc = aug_set(her->aug, exclPath.c_str(), her->excl.c_str());
            if (AUG_NOERROR != rc)
                return;
        }

        rc = aug_load(her->aug);
        if (AUG_NOERROR != rc)
            return;
    }
}

void heraclesAfter(uv_work_t* req)
{
    HandleScope scope;
    HeraclesUV *her = static_cast<HeraclesUV*>(req->data);

    if (AUG_NOERROR == aug_error(her->aug)) {
        Local<Value> argv[] = {LibAugeas::New(her->aug)};
        TryCatch try_catch;
        her->callback->Call(Context::GetCurrent()->Global(), 1, argv);
        if (try_catch.HasCaught()) {
            node::FatalException(try_catch);
        }
    }

    her->callback.Dispose();
    delete her;
}

Handle<Value> heracles(const Arguments& args)
{
    HandleScope scope;
    Local<Function> callback;

    if (args.Length() == 1) {
        if (!args[0]->IsFunction()) {
            ThrowException(Exception::TypeError(String::New("Single argument must be a function")));
            return scope.Close(Undefined());
        }
        callback = Local<Function>::Cast(args[0]);
    }

    HeraclesUV *her = new HeraclesUV();
    her->request.data = her;
    her->callback = Persistent<Function>::New(callback);

    uv_queue_work(uv_default_loop(), &her->request,
                  heraclesWork, heraclesAfter);

    return scope.Close(Undefined());
}



void init(Handle<Object> target)
{
    LibAugeas::Init(target);

    target->Set(String::NewSymbol("heracles"), FunctionTemplate::New(heracles)->GetFunction());
}

NODE_MODULE(libaugeas, init)

