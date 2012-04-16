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

class LibAugeas : public node::ObjectWrap {
public:
    static void Init(Handle<Object> target);

protected:
    augeas * m_aug;
    LibAugeas();
    ~LibAugeas();

    static Handle<Value> New(const Arguments& args);

    static Handle<Value> get        (const Arguments& args);
    static Handle<Value> set        (const Arguments& args);
    static Handle<Value> setm       (const Arguments& args);
    static Handle<Value> rm         (const Arguments& args);
    static Handle<Value> mv         (const Arguments& args);
    static Handle<Value> save       (const Arguments& args);
    static Handle<Value> nmatch     (const Arguments& args);
    static Handle<Value> load       (const Arguments& args);
    static Handle<Value> loadFile   (const Arguments& args);
    static Handle<Value> saveFile   (const Arguments& args);
    static Handle<Value> insert     (const Arguments& args);
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
    _NEW_METHOD(load);
    _NEW_METHOD(loadFile);
    _NEW_METHOD(saveFile);
    _NEW_METHOD(insert);

    //TODO:
    // _NEW_METHOD(match);

    Persistent<Function> constructor =
        Persistent<Function>::New(tpl->GetFunction());

    target->Set(String::NewSymbol("Augeas"), constructor);
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
        ThrowException(Exception::Error(String::New("aug_insert() failed")));
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

struct LoadFileUV {
    uv_work_t request;
    Persistent<Function> callback;
    std::string lens;
    std::string incl;
    augeas *m_aug;
    int ret;
    std::string msg;
    std::map<std::string,std::string> msgMap;
};

void blockingLoadFile(uv_work_t * req)
{
    char **matches;
    const char *val;
    int mres;
    LoadFileUV *lfuv = (LoadFileUV *) req->data;

    std::string basePath = "/augeas/load/" + lfuv->lens;
    std::string lensPath = basePath + "/lens";
    std::string inclPath = basePath + "/incl";
    std::string lensVal = lfuv->lens + ".lns";

    lfuv->ret = aug_set(lfuv->m_aug, lensPath.c_str(), lensVal.c_str());
    if (lfuv->ret != 0) {
        lfuv->msg = "Cannot set lens";
        return;
    }

    lfuv->ret = aug_set(lfuv->m_aug, inclPath.c_str(), lfuv->incl.c_str());
    if (lfuv->ret != 0) {
        lfuv->msg = "Cannot set file";
        return;
    }

    lfuv->ret = aug_load(lfuv->m_aug);
    if (lfuv->ret != 0) {
        lfuv->msg = "Cannot load provided lens or file";
        return;
    }
    std::string errPath = "/augeas/load/" + lfuv->lens + "/error";
    if (aug_get(lfuv->m_aug, errPath.c_str(), &val)) {
        lfuv->ret = -1;
        lfuv->msg = val;
        return;
    }

    errPath = "/augeas/files" + lfuv->incl + "/error";
    mres = aug_match(lfuv->m_aug, errPath.c_str(), &matches);
    if (mres) {
        if (aug_get(lfuv->m_aug,
		    std::string(errPath + "/line").c_str(), &val)) {
		lfuv->msg = lfuv->incl + ":" + val;
	}
        if (aug_get(lfuv->m_aug,
		    std::string(errPath + "/message").c_str(), &val)) {
		lfuv->msg = lfuv->msg + ": " + val;
	}
	lfuv->ret = -1;
	free(matches);
	return;
    }

    std::string matchPath = "/files" + lfuv->incl + "/*";
    FILE *out = tmpfile();
    char line[256];
    aug_print(lfuv->m_aug, out, matchPath.c_str());
    rewind(out);
    while(fgets(line, 256, out) != NULL) {
        // remove end of line
        line[strlen(line) - 1] = '\0';
        std::string s = line;;
        // skip comments
	if (s.find("#comment") != std::string::npos)
            continue;
	s = s.substr(matchPath.length() - 1);
        // split by '=' sign
	size_t eqpos = s.find(" = ");
	if (eqpos == std::string::npos)
            continue;
        // extract key and value
	std::string key = s.substr(0, eqpos);
	std::string value = s.substr(eqpos + 3);
        // remove '"' sign from around value
	value.erase(value.begin());
	value.erase(value.end() - 1);
	lfuv->msgMap[key] = value;
    }
    fclose(out);
    lfuv->ret = 0;
}

void afterLoadFile(uv_work_t *req)
{
    HandleScope scope;
    LoadFileUV *lfuv = (LoadFileUV *) req->data;

    if(lfuv->ret != 0) {
        Local<Value> argv[] =  {
            Local<Value>::New(Integer::New(lfuv->ret)),
            Local<Value>::New(String::New(lfuv->msg.c_str()))
        };

        TryCatch try_catch;
        lfuv->callback->Call(Context::GetCurrent()->Global(), 2, argv);
        if (try_catch.HasCaught()) {
            node::FatalException(try_catch);
        }
    } else {
        std::map<std::string,std::string>::iterator it;
        Local<Object> obj = Object::New();

        for (it = lfuv->msgMap.begin(); it != lfuv->msgMap.end(); it++) {
            obj->Set(Local<String>::New(String::New((*it).first.c_str())),
                     Local<String>::New(String::New((*it).second.c_str())));
        }
        Local<Value> argv[] =  {
            Local<Value>::New(Integer::New(lfuv->ret)),
	    obj
        };

        TryCatch try_catch;
        lfuv->callback->Call(Context::GetCurrent()->Global(), 2, argv);
        if (try_catch.HasCaught()) {
            node::FatalException(try_catch);
        }
    }

    lfuv->callback.Dispose();
    delete lfuv;

    scope.Close(Undefined());
}

/*
 * Asyncrhonous load of lens and conf file. Aggregation of
 * set lens, set incl, load and get
 */
Handle<Value> LibAugeas::loadFile(const Arguments& args)
{
    HandleScope scope;

    if (!args[0]->IsObject() || !args[1]->IsFunction())
        return (ThrowException(Exception::TypeError(
                                    String::New("Bad arguments"))));

    LibAugeas *augobj = ObjectWrap::Unwrap<LibAugeas>(args.This());
    Local<Object> obj = args[0]->ToObject();
    String::Utf8Value lens(obj->Get(Local<String>(String::New("lens"))));
    String::Utf8Value incl(obj->Get(Local<String>(String::New("file"))));
    Local<Function> callback = Local<Function>::Cast(args[1]);

    LoadFileUV *lfuv = new LoadFileUV();
    lfuv->request.data = lfuv;
    lfuv->callback = Persistent<Function>::New(callback);
    lfuv->m_aug = augobj->m_aug;
    lfuv->lens = *lens;
    lfuv->incl = *incl;

    assert(uv_queue_work(uv_default_loop(), &lfuv->request,
           blockingLoadFile, afterLoadFile) == 0);

    return scope.Close(Undefined());
}

struct SaveFileUV {
    uv_work_t request;
    Persistent<Function> callback;
    std::map<std::string,std::string> valMap;
    std::string lens;
    std::string incl;
    augeas *m_aug;
    int ret;
    std::string msg;
};

void blockingSaveFile(uv_work_t * req)
{
    char **matches;
    const char *val;
    int mres;
    SaveFileUV *lfuv = (SaveFileUV *) req->data;

    std::string basePath = "/augeas/load/" + lfuv->lens;
    std::string lensPath = basePath + "/lens";
    std::string inclPath = basePath + "/incl";
    std::string lensVal = lfuv->lens + ".lns";
    std::string filesPath = "/files" + lfuv->incl;

    mres = aug_match(lfuv->m_aug, inclPath.c_str(), &matches);
    if (!mres) {

        lfuv->ret = aug_set(lfuv->m_aug, lensPath.c_str(), lensVal.c_str());
        if (lfuv->ret != 0) {
            lfuv->msg = "Cannot set lens";
            return;
        }

        lfuv->ret = aug_set(lfuv->m_aug, inclPath.c_str(), lfuv->incl.c_str());
        if (lfuv->ret != 0) {
            lfuv->msg = "Cannot set file";
            return;
        }

        lfuv->ret = aug_load(lfuv->m_aug);
        if (lfuv->ret != 0) {
            lfuv->msg = "Cannot load provided lens or file";
            return;
        }
        std::string errPath = "/augeas/load/" + lfuv->lens + "/error";
        if (aug_get(lfuv->m_aug, errPath.c_str(), &val)) {
            lfuv->ret = -1;
            lfuv->msg = val;
            return;
        }

        errPath = "/augeas" + filesPath + "/error";
        mres = aug_match(lfuv->m_aug, errPath.c_str(), &matches);
        if (mres) {
            if (aug_get(lfuv->m_aug,
                std::string(errPath + "/line").c_str(), &val)) {
	        lfuv->msg = lfuv->incl + ":" + val;
            }
            if (aug_get(lfuv->m_aug,
	        std::string(errPath + "/message").c_str(), &val)) {
	        lfuv->msg = lfuv->msg + ": " + val;
            }
            lfuv->ret = -1;
            free(matches);
            return;
        }
    }

    std::map<std::string,std::string>::iterator it;

    for (it = lfuv->valMap.begin(); it != lfuv->valMap.end(); it++) {
        std::string path = filesPath + "/" + (*it).first;
        lfuv->ret = aug_set(lfuv->m_aug, path.c_str(), (*it).second.c_str());
        if (lfuv->ret != 0) {
            lfuv->msg = "Cannot set file";
            return;
        }
    }

    lfuv->ret = aug_save(lfuv->m_aug);
    if (lfuv->ret != 0) {
        std::string errPath = "/augeas" + filesPath + "/error";
        if (aug_get(lfuv->m_aug,
            std::string(errPath + "/line").c_str(), &val)) {
            lfuv->msg = lfuv->incl + ":" + val;
        }
        if (aug_get(lfuv->m_aug,
            std::string(errPath + "/message").c_str(), &val)) {
            lfuv->msg = lfuv->msg + ": " + val;
        }
        lfuv->ret = -1;
        return;
    }

    lfuv->ret = 0;
}

void afterSaveFile(uv_work_t *req)
{
    HandleScope scope;
    SaveFileUV *lfuv = (SaveFileUV *) req->data;

    Local<Value> argv[] =  {
        Local<Value>::New(Integer::New(lfuv->ret)),
        Local<Value>::New(String::New(lfuv->msg.c_str()))
    };

    TryCatch try_catch;
    lfuv->callback->Call(Context::GetCurrent()->Global(), 2, argv);
    if (try_catch.HasCaught()) {
        node::FatalException(try_catch);
    }

    lfuv->callback.Dispose();
    delete lfuv;

    scope.Close(Undefined());
}

/*
 * Asyncrhonous save of conf file. Aggregation of set values and save
 */
Handle<Value> LibAugeas::saveFile(const Arguments& args)
{
    HandleScope scope;

    if (!args[0]->IsObject() || !args[1]->IsObject() || !args[2]->IsFunction())
        return (ThrowException(Exception::TypeError(
                                    String::New("Bad arguments"))));

    LibAugeas *augobj = ObjectWrap::Unwrap<LibAugeas>(args.This());

    Local<Object> obj = args[0]->ToObject();
    String::Utf8Value lens(obj->Get(Local<String>(String::New("lens"))));
    String::Utf8Value incl(obj->Get(Local<String>(String::New("file"))));

    Local<Object> setObj = args[1]->ToObject();

    Local<Function> callback = Local<Function>::Cast(args[2]);

    SaveFileUV *lfuv = new SaveFileUV();
    lfuv->request.data = lfuv;
    lfuv->callback = Persistent<Function>::New(callback);
    lfuv->m_aug = augobj->m_aug;
    lfuv->lens = *lens;
    lfuv->incl = *incl;

    Local<Array> propertyNames = setObj->GetPropertyNames();
    for (int i = 0; i < propertyNames->Length(); i++) {
        Local<Value> propKey = propertyNames->Get(Int32::New(i));
	String::Utf8Value key(propKey);
        String::Utf8Value value(setObj->Get(propKey));
	lfuv->valMap[*key] = *value;
    }

    assert(uv_queue_work(uv_default_loop(), &lfuv->request,
           blockingSaveFile, afterSaveFile) == 0);

    return scope.Close(Undefined());
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

