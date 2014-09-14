#include <node.h>
#include <node_buffer.h>
#include <nan.h>
#include <xbps.h>

#define JS_XBPS_CLASSNAME "Xbps"
#define JS_XBPS_EXPORT "Xbps"
#define MEMBER JsXbps *self = ObjectWrap::Unwrap<JsXbps>(args.This())
#define LENGTH(x) sizeof(x)/sizeof(x[0])
#define SIMPLE_WRAPPER_RV(x, f) \
	static NAN_METHOD(x) { \
		NanScope(); \
		MEMBER; \
		int rv = 0; \
		if((rv = f)) { \
			NanThrowError(strerror(rv)); \
		} \
		NanReturnValue(NanUndefined()); \
	}
#define SIMPLE_WRAPPER(x, f) \
	static NAN_METHOD(x) { \
		NanScope(); \
		MEMBER; \
		f; \
		NanReturnValue(NanUndefined()); \
	}

using namespace v8;

class JsXbps : node::ObjectWrap {
private:
	xbps_handle xh;
	Persistent<Object> jsObj;
	char *target_arch;

	JsXbps() {
		memset(&this->xh, 0, sizeof(xh));
		this->target_arch = NULL;
	}
	~JsXbps() {
		xbps_end(&this->xh);
		NanDisposePersistent(this->jsObj);
	}

	static const char *stateStr(xbps_state_t state) {
		switch (state) {
			case XBPS_STATE_TRANS_DOWNLOAD:
				return (const char*)"TRANS_DOWNLOAD";
			case XBPS_STATE_TRANS_VERIFY:
				return (const char*)"TRANS_VERIFY";
			case XBPS_STATE_TRANS_RUN:
				return (const char*)"TRANS_RUN";
			case XBPS_STATE_TRANS_CONFIGURE:
				return (const char*)"TRANS_CONFIGURE";
			case XBPS_STATE_PKGDB:
				return (const char*)"PKGDB";
			case XBPS_STATE_REPOSYNC:
				return (const char*)"REPOSYNC";
			case XBPS_STATE_VERIFY:
				return (const char*)"VERIFY";
			case XBPS_STATE_CONFIG_FILE:
				return (const char*)"CONFIG_FILE";
			case XBPS_STATE_REMOVE:
				return (const char*)"REMOVE";
			case XBPS_STATE_CONFIGURE:
				return (const char*)"CONFIGURE";
			case XBPS_STATE_CONFIGURE_DONE:
				return (const char*)"CONFIGURE_DONE";
			case XBPS_STATE_UNPACK:
				return (const char*)"UNPACK";
			case XBPS_STATE_INSTALL:
				return (const char*)"INSTALL";
			case XBPS_STATE_DOWNLOAD:
				return (const char*)"DOWNLOAD";
			case XBPS_STATE_UPDATE:
				return (const char*)"UPDATE";
			case XBPS_STATE_REMOVE_FILE:
				return (const char*)"REMOVE_FILE";
			case XBPS_STATE_REMOVE_FILE_OBSOLETE:
				return (const char*)"REMOVE_FILE_OBSOLETE";
			case XBPS_STATE_INSTALL_DONE:
				return (const char*)"INSTALL_DONE";
			case XBPS_STATE_UPDATE_DONE:
				return (const char*)"UPDATE_DONE";
			case XBPS_STATE_REMOVE_DONE:
				return (const char*)"REMOVE_DONE";
			case XBPS_STATE_PKGDB_DONE:
				return (const char*)"PKGDB_DONE";
			case XBPS_STATE_REPO_KEY_IMPORT:
				return (const char*)"REPO_KEY_IMPORT";
			case XBPS_STATE_SHOW_INSTALL_MSG:
				return (const char*)"SHOW_INSTALL_MSG";
			case XBPS_STATE_UNPACK_FILE_PRESERVED:
				return (const char*)"UNPACK_FILE_PRESERVED";
			case XBPS_STATE_UNPACK_FAIL:
				return (const char*)"UNPACK_FAIL";
			case XBPS_STATE_UPDATE_FAIL:
				return (const char*)"UPDATE_FAIL";
			case XBPS_STATE_CONFIGURE_FAIL:
				return (const char*)"CONFIGURE_FAIL";
			case XBPS_STATE_REMOVE_FAIL:
				return (const char*)"REMOVE_FAIL";
			case XBPS_STATE_VERIFY_FAIL:
				return (const char*)"VERIFY_FAIL";
			case XBPS_STATE_DOWNLOAD_FAIL:
				return (const char*)"DOWNLOAD_FAIL";
			case XBPS_STATE_REPOSYNC_FAIL:
				return (const char*)"REPOSYNC_FAIL";
			case XBPS_STATE_CONFIG_FILE_FAIL:
				return (const char*)"CONFIG_FILE_FAIL";
			case XBPS_STATE_REMOVE_FILE_FAIL:
				return (const char*)"REMOVE_FILE_FAIL";
			case XBPS_STATE_REMOVE_FILE_HASH_FAIL:
				return (const char*)"REMOVE_FILE_HASH_FAIL";
			case XBPS_STATE_REMOVE_FILE_OBSOLETE_FAIL:
				return (const char*)"REMOVE_FILE_OBSOLETE_FAIL";
			default:
				return (const char*)"UNKNOWN";
		}
	}

	static int stateCb(const struct xbps_state_cb_data *xscd, void *data) {
		int rv = 0;
		JsXbps *self = reinterpret_cast<JsXbps *>(data);
		Handle<Object> obj = NanNew<Object>();
		obj->Set(NanNew<String>("state"), NanNew<String>(stateStr(xscd->state)));
		obj->Set(NanNew<String>("arg"), NanNew<String>(xscd->arg));
		obj->Set(NanNew<String>("desc"), NanNew<String>(xscd->desc));
		obj->Set(NanNew<String>("err"), NanNew<Integer>(xscd->err));
		Handle<Value> argv[] = {
			NanNew<String>("state"), obj
		};
		self->jsObj->Get(NanNew<String>("emit")).As<Function>()->Call(self->jsObj, LENGTH(argv), argv);
		return rv;
	}

	static void fetchCb(const struct xbps_fetch_cb_data *xfcd, void *data) {
		unsigned int i;
		JsXbps *self = reinterpret_cast<JsXbps *>(data);
		const bool states[] = { xfcd->cb_start, xfcd->cb_update, xfcd->cb_end };
		const char *events[] = { "fetchstart", "fetchupdate", "fetchend" };
		for(i = 0; i < LENGTH(states); i++) {
			if(!states[i]) break;

			Handle<Object> obj = NanNew<Object>();
			obj->Set(NanNew<String>("dloaded"), NanNew<Integer>(xfcd->file_dloaded));
			obj->Set(NanNew<String>("name"), NanNew<String>(xfcd->file_name));
			obj->Set(NanNew<String>("offset"), NanNew<Integer>(xfcd->file_offset));
			obj->Set(NanNew<String>("size"), NanNew<Integer>(xfcd->file_size));
			Handle<Value> argv[] = {
				NanNew<String>(events[i]), obj
			};
			self->jsObj->Get(NanNew<String>("emit")).As<Function>()->Call(self->jsObj, LENGTH(argv), argv);
		}
	}

	static void unpackCb(const struct xbps_unpack_cb_data *xucd, void *data) {
		JsXbps *self = reinterpret_cast<JsXbps *>(data);
		Handle<Object> obj = NanNew<Object>();
		obj->Set(NanNew<String>("entry"), NanNew<String>(xucd->entry));
		obj->Set(NanNew<String>("entry_extract_count"), NanNew<Integer>(xucd->entry_extract_count));
		obj->Set(NanNew<String>("entry_is_conf"), NanNew<Boolean>(xucd->entry_is_conf));
		obj->Set(NanNew<String>("entry_size"), NanNew<Integer>(xucd->entry_size));
		obj->Set(NanNew<String>("entry_total_count"), NanNew<Integer>(xucd->entry_total_count));
		obj->Set(NanNew<String>("pkgver"), NanNew<String>(xucd->pkgver));
		Handle<Value> argv[] = {
			NanNew<String>("unpack"), obj
		};
		self->jsObj->Get(NanNew<String>("emit")).As<Function>()->Call(self->jsObj, LENGTH(argv), argv);
	}

public:
	static NAN_METHOD(Fetch_file) {
		NanScope();
		MEMBER;
		int rv = 0;
		switch(args.Length()) {
		case 2:
			rv = xbps_fetch_file(&self->xh,
					*NanAsciiString(args[0].As<String>()),
					*NanAsciiString(args[1].As<String>()));
			break;
		case 3:
			rv = xbps_fetch_file_dest(&self->xh,
					*NanAsciiString(args[0].As<String>()),
					*NanAsciiString(args[1].As<String>()),
					*NanAsciiString(args[2].As<String>()));
			break;
		case 4:
			rv = xbps_fetch_delta(&self->xh,
					*NanAsciiString(args[0].As<String>()),
					*NanAsciiString(args[1].As<String>()),
					*NanAsciiString(args[2].As<String>()),
					*NanAsciiString(args[3].As<String>()));
			break;
		default:
			NanThrowError("Function must have 2, 3, or 4 arguments");
		}
		if(rv != 0)
			NanThrowError(xbps_fetch_error_string());
		NanReturnValue(NanUndefined());
	}

	SIMPLE_WRAPPER_RV(Configure_packages,
			xbps_configure_packages(&self->xh))
	SIMPLE_WRAPPER_RV(Configure_pkg,
			xbps_configure_pkg(&self->xh, *NanAsciiString(args[0]), args[1]->BooleanValue(), args[2]->BooleanValue()))
	SIMPLE_WRAPPER_RV(Pkgdb_lock,
			xbps_pkgdb_lock(&self->xh))
	SIMPLE_WRAPPER(Pkgdb_unlock,
			xbps_pkgdb_unlock(&self->xh))
	SIMPLE_WRAPPER_RV(Pkgdb_update,
			xbps_pkgdb_update(&self->xh, args[0]->BooleanValue()))

	static NAN_GETTER(GetTargetArch) {
		NanScope();
		MEMBER;
		if(self->xh.target_arch)
			NanReturnValue(NanNew<String>(self->xh.target_arch));
		else
			NanReturnValue(NanNull());
	}

	static NAN_SETTER(SetTargetArch) {
		NanScope();
		MEMBER;
		if(self->target_arch)
			delete self->target_arch;
		NanAsciiString str(value.As<String>());
		self->xh.target_arch = self->target_arch = new char[str.Size()];
		strcpy(self->target_arch, *str);
	}

	static NAN_METHOD(New) {
		int rv;
		NanScope();
		JsXbps *self = new JsXbps();
		if((rv = xbps_init(&self->xh)) != 0) {
			NanThrowError(strerror(rv));
		}

		self->Wrap(args.This());
		self->xh.state_cb = JsXbps::stateCb;
		self->xh.fetch_cb = JsXbps::fetchCb;
		self->xh.unpack_cb = JsXbps::unpackCb;
		self->xh.unpack_cb_data =
		self->xh.fetch_cb_data =
		self->xh.state_cb_data = self;
		NanAssignPersistent(self->jsObj, args.This());

		NanReturnValue(NanUndefined());
	}

	static void Init(Handle<Object> exports) {
		Local<FunctionTemplate> tpl = NanNew<FunctionTemplate>(JsXbps::New);
		tpl->InstanceTemplate()->SetInternalFieldCount(1);
		tpl->InstanceTemplate()->SetAccessor(NanNew<String>("target_arch"), GetTargetArch, SetTargetArch);
		tpl->SetClassName(NanNew<String>(JS_XBPS_CLASSNAME));

		exports->Set(NanNew<String>(JS_XBPS_EXPORT), tpl->GetFunction());
	}
};

NODE_MODULE(xbps, JsXbps::Init)
