/// Code extracted from wrap_com.cpp from JSDB by Shanti Rao (see http://jsdb.org)

#include <stdafx.h>
#include "active_x.h"

#include <js_utils/js_object_helper.h>
#include <js_engine/js_to_native_converter.h>
#include <js_engine/native_to_js_converter.h>
#include <js_engine/js_to_native_invoker.h>

#pragma warning( push )  
#pragma warning( disable : 4251 ) // dll interface warning
#pragma warning( disable : 4996 ) // C++17 deprecation warning
#include <jsapi.h>
#pragma warning( pop )  

#include <oleauto.h>

#include <vector>
#include <string>

#define JS_GetStringChars(s)  JS_GetStringCharsZ(cx,s)
#define NATIVE(name) JSBool name(JSContext *cx, uintN argc, jsval *vp)

#define GETARGS \
jsval* argv = JS_ARGV(cx, vp)

#define ARGV(x) JS_ARGV(cx,vp)[x]
//JSObject* obj = JS_THIS_OBJECT(cx, vp)

#define RVAL (&JS_RVAL(cx, vp))

#define WRAP(type,name) NATIVE(type ## _ ## name)

#define CALL(type,name) \
type ## _ ## name(cx, argc, vp)

#define GETUTF8(x) \
 JSString* j8 ## x = argc > x ? JS_ValueToString(cx,ARGV(x)) : 0; \
 size_t j8 ## x ## length = (j8 ## x) ? JS_GetStringEncodingLength(cx,j8 ## x) : 0; \
  TStr u ## x( j8 ## x ## length); \
  JS_EncodeStringToBuffer(j8 ## x , u##x.str, j8 ## x ## length)

#define GETUCS2(x) \
 JSString* j ## x = argc > x ? JS_ValueToString(cx,ARGV(x)) : 0; \
 uint16* s ## x = (j ## x) ? (uint16*)JS_GetStringChars(j ## x) : (uint16*)0

#define RETVAL(value) JS_SET_RVAL(cx,vp,value)

#define GETTHIS  JSObject* obj = JS_THIS_OBJECT(cx, vp)

//for property getters/setters
#define GETOBJ2(class,type,name) \
 jsval*vp = rval;\
 type * name = NULL; \
 if (GETCLASS != class ## _Class() ) return JSBadClass(cx);\
 JSPointer<type> *ptr_ ## name = (JSPointer<type>*)JS_GetPrivate(cx,obj); \
 if (ptr_ ## name) name = *ptr_ ## name; \
 if (!name) return JS_FALSE

#define GETOBJ(class,type,name) \
 GETTHIS;\
 type * name = NULL; \
 if (GETCLASS != class ## _Class() ) return JSBadClass(cx);\
 JSPointer<type> *ptr_ ## name = (JSPointer<type>*)JS_GetPrivate(cx,obj); \
 if (ptr_ ## name) name = *ptr_ ## name; \
 if (!name) return JS_FALSE

#define CONSTRUCTOR(x,t,ad,parent) \
  RETOBJ(x ## _Object(cx,t,ad,parent))

/* Can we number of memory copies for string arguments?
class JSStr
{   public:
JSContext* cx;
char* str;
JSStr(JSContext* c,char* s): cx(c), str(s){}
~JSStr() {JS_free(cx,str);}
char* operator () {return str;}
};*/



#define ENTERNATIVE(cx) JSBlockGC __BlockGC(cx); JSBlocker __Blocker(cx); TList<JSRoot> __Roots


/* Version 1.8 doesn't need automatic rooting, because GC is blocked within a native method call */
/* SQLite still needs the root facility for storing callback functions*/
#define ROOT(x) (x)


#define ISDBL(x) JSVAL_IS_DOUBLE(ARGV(x))
#define ISINT(x) JSVAL_IS_INT(ARGV(x))
#define ISSTR(x) JSVAL_IS_STRING(ARGV(x))
#define ISBOOL(x) JSVAL_IS_BOOLEAN(ARGV(x))

#define ISOBJ(x) JSVAL_IS_OBJECT(ARGV(x))

#define TOBOOL(x,y) JS_ValueToBoolean(cx,ARGV(x),&(y))
#define TOINT(x,y) JS_ValueToInt32(cx,ARGV(x),&(y))
#define TODBL(x,y) JS_ValueToNumber(cx,ARGV(x),&(y))

#define GETPRIVATE(x,obj) (((JSPointer<x>*)JS_GetPrivate(cx,obj))->P)
#define GETPOINTER (((JSPointerBase*)JS_GetPrivate(cx,obj)))

#define GETCLASS JS_GetClass(cx,obj)

#define SETPRIVATE(obj,x,t,ad,parent) \
 JS_SetPrivate(cx,obj,new JSPointer<x>(parent,t,ad))

#define DELPRIVATE(x) \
JSPointer<x> * t = \
   (JSPointer<x>*)JS_GetPrivate(cx,obj);\
 if (t) delete t; t=0; JS_SetPrivate(cx,obj,NULL)

#define CLOSEPRIVATE(class,x) \
if (GETCLASS != class ## _Class() ) return JSBadClass(cx);\
 JSPointer<x> * t = (JSPointer<x>*)JS_GetPrivate(cx,obj);\
 if (t) t->Close()

#define MAKENEW(name) \
obj = ROOT(JS_NewObject(cx, JS_GetClass(cx,Env->o##name),Env->o##name, NULL));\
JS_DefineFunctions(cx,obj,name ## _functions);\
JS_DefineProperties(cx,obj,name ## _properties)

#define INITCLASS(name) \
 Env->o##name = JS_InitClass(cx, obj, NULL, name ## _Class(),\
 name ## _ ## name, 0,\
 name ## _properties, name ## _functions,NULL,name ## _fnstatic);

#define GETREC(x,y)  \
{JSObject* jx = x < argc && JSVAL_IS_OBJECT(ARGV(x))? JSVAL_TO_OBJECT(ARGV(x)) : NULL; \
 if (jx) {\
  if (JS_InstanceOf(cx,jx,Record_Class(),0)) \
   { y = GETPRIVATE(TNameValueList,jx); }\
  else \
   { y = y ## AutoDelete = new TParameterList; ObjectToList(cx, jx, *y); }\
 } else y = 0;\
}

#define GETREC1(x,y)  \
{JSObject* jx = x < argc && JSVAL_IS_OBJECT(ARGV(x))? JSVAL_TO_OBJECT(ARGV(x)) : NULL; \
 if (jx) {\
  if (JS_InstanceOf(cx,jx,Record_Class(),0)) \
   y = GETPRIVATE(TNameValueList,jx); \
 } else y=0;\
}

#define GETFORM(x,y)  \
{JSObject* jx = x < argc && JSVAL_IS_OBJECT(ARGV(x))? JSVAL_TO_OBJECT(ARGV(x)) : NULL; \
 if (jx) {if (JS_InstanceOf(cx,jx,Form_Class(),0)) \
  y = GETPRIVATE(EZFForm,jx);} \
 else y = NULL;\
}

#define GETTABLE(x,y)  \
{JSObject* jx = x < argc && JSVAL_IS_OBJECT(ARGV(x))? JSVAL_TO_OBJECT(ARGV(x)) : NULL; \
 if (jx) {if (JS_InstanceOf(cx,jx,Table_Class(),0)) \
  y = GETPRIVATE(DataTable,jx);} \
 else y = NULL;\
}

#define GETFILE(x,y)  \
{JSObject* jx = x < argc&& JSVAL_IS_OBJECT(ARGV(x)) ? JSVAL_TO_OBJECT(ARGV(x)) : NULL; \
 if (jx && JS_InstanceOf(cx,jx,Stream_Class(),0)) \
  y = GETPRIVATE(Stream,jx); \
 else y = NULL;\
}

#define GETSTRN(x)\
 JSString* j ## x = argc > x ? JS_ValueToString(cx,ARGV(x)) : 0; \
 const char* s ## x = j ## x ? JS_GetStringBytes(j ## x) : 0; \
 size_t l ## x = j ## x ? JS_GetStringLength(j ## x) : 0

#define GETSTRING(x) \
 {j ## x = argc > x ? JS_ValueToString(cx,ARGV(x)) : 0; \
  s ## x = (j ## x) ? JS_GetStringBytes(j ## x) : 0;}

// JSString* j ## x = argc > x ? JS_ValueToString(cx,argv[x]) : 0;
//  TStr s ## x( (j ## x) ? JS_GetStringBytes(j ## x) : (char*)0)

#define INT(x) JSVAL_TO_INT(ARGV(x))
//doesn't work on 1.85
#define STR(x) JS_GetStringBytes(JSVAL_TO_STRING(ARGV(x)))
#define WSTR(x) JS_GetStringCharsZ(cx,JSVAL_TO_STRING(ARGV(x)))
//#define UTF8STR(x) TStr((const uint16*)JS_GetStringCharsZ(cx,JSVAL_TO_STRING(ARGV(x))))
//#define DBL(x) JSVAL_TO_DOUBLE(ARGV(x))
//#define TF(x) JSVAL_TO_BOOLEAN(ARGV(x))

//RSLIB converts UTF-8 to UCS-2 to return a unicode string.
#define RETSTRWC(c) \
{WStr x(c); JSString*str = JS_NewUCStringCopyZ(cx,(jschar*)(wchar_t*)x); \
 if (!str) return JS_FALSE; \
 RETVAL(STRING_TO_JSVAL(str)); \
 return JS_TRUE; }

#define RETSTRW(x) \
{JSString*str = JS_NewUCStringCopyZ(cx,(jschar*)(wchar_t*)x); \
 if (!str) return JS_FALSE; \
 RETVAL(STRING_TO_JSVAL(str)); \
 return JS_TRUE; }

#define RETSTRWN(x,n) \
{JSString*str = JS_NewUCStringCopyN(cx,x,n); \
 if (!str) return JS_FALSE; \
 RETVAL(STRING_TO_JSVAL(str)); \
 return JS_TRUE; }

#define RETSTR(x) \
{JSString*str = JS_NewStringCopyZ(cx,x); \
 if (!str) return JS_FALSE; \
 RETVAL(STRING_TO_JSVAL(str)); \
 return JS_TRUE; }

#define RETSTRN(x,n) \
{JSString*str = JS_NewStringCopyN(cx,x,n); \
 if (!str) return JS_FALSE; \
 RETVAL(STRING_TO_JSVAL(str)); \
 return JS_TRUE; }

#define RETBOOL(x) {RETVAL(BOOLEAN_TO_JSVAL((x)?JS_TRUE:JS_FALSE)); return JS_TRUE ;}

#define RETOBJ(x) {RETVAL(OBJECT_TO_JSVAL(x)); return JS_TRUE;}

//#define RETINT(x) {*rval = INT_TO_JSVAL(x); return JS_TRUE;}

#define RETINT(y) \
{\
 int x = y;\
 RETVAL(INT_TO_JSVAL(x));\
 return JS_TRUE;\
}

#define RETUINT(x) RETVAL(UINT_TO_JSVAL(x));

#define MAYBEGC \
 if (++Env->GCTimer > (2<<10)) {Env->GCTimer=0;JS_MaybeGC(cx);}

#define GETENV \
 JSDBEnvironment* Env = (JSDBEnvironment*)JS_GetContextPrivate(cx);\
 MAYBEGC

#define WRITELN(x) {if (Env->out) Env->out->writestr(x,"\n");}

#define ERR_COUNT(class,name) \
 {JS_ReportError(cx,"Wrong number of parameters in call to %s.%s",#class,#name); \
 return JS_FALSE; }

#define ERR_TYPE(class,name,index,type) \
 {JS_ReportError(cx,"Expected a %s in parameter %d for %s.%s",#type,index,#class,#name); \
 return JS_FALSE; }

#define ERR_XDB(class,x) \
 { JS_SetPendingException(cx,STRING_TO_JSVAL(JS_NewStringCopyZ(cx,TStr(#class " (", x.why(),":",x.info(), ")"))));\
  RETVAL(OBJECT_TO_JSVAL(NULL)); return JS_FALSE; }

#define ERR_MSG(class,name,msg) \
 { JS_SetPendingException(cx,STRING_TO_JSVAL(JS_NewStringCopyZ(cx,TStr(#class ":" #name " (", msg, ")"))));\
  RETVAL(OBJECT_TO_JSVAL(NULL)); return JS_FALSE; }
/*JS_ReportError(cx,"%s.%s: %s",#class,#name,msg); */

/*#define WARN_MSG(class,name,msg) \
{JS_ReportError(cx,"%s.%s: %s",#class,#name,msg); \
return JS_TRUE; } */

#define I_WRAP_S(class,type,name) WRAP(type,name)\
{ \
 if (argc != 1) ERR_COUNT(type,name); \
 if (!ISSTR(0)) ERR_TYPE(type,name,1,string); \
 GETOBJ(class,type,t); \
 RETINT(t->name(STR(0))); \
}

#define B_WRAP_S(class,type,name) WRAP(type,name)\
{ \
 if (argc != 1) ERR_COUNT(type,name); \
 if (!ISSTR(0)) ERR_TYPE(type,name,1,string); \
 GETOBJ(class,type,t); \
 RETBOOL(t->name(TStr(WSTR(0)))); \
}

#define B_WRAP_SET_R(class,type,list,name) WRAP(type,_set ## name)\
{ \
 if (argc != 2) ERR_COUNT(type,name); \
 int32 index; TNameValueList * r1; \
 if (!TOINT(0,index)) ERR_TYPE(type,name,1,integer); \
 TPointer<TParameterList> r1AutoDelete; \
 GETREC(1,r1); if (!r1) ERR_TYPE(type,name,2,record); \
 GETOBJ(class,type,t);\
 if (t->list[index] == NULL) RETBOOL(false);\
 if (&t->list[index]->name == r1) RETBOOL(true);\
 t->list[index]->name.Clear();\
 t->list[index]->name.Append(*r1);\
 RETBOOL(true);\
}

#define R_WRAP_GET(class,type,list,name) WRAP(type,_get ## name)\
{ \
 if (argc != 1) ERR_COUNT(type,name); \
 int32 index; TNameValueList * r1=0; \
 if (!TOINT(0,index)) ERR_TYPE(type,name,1,integer); \
 if (argc == 2)\
 {TPointer<TParameterList> r1AutoDelete; \
 GETREC(1,r1); if (!r1) ERR_TYPE(type,name,2,record); }\
 GETOBJ(class,type,t);\
 if (t->list[index] == NULL) RETOBJ(NULL); \
 if (r1) \
  {*r1 = t->list[index]->Responses; RETOBJ(NULL); }\
 else \
  {RETOBJ(Record_Object(cx,&t->list[index]->name,false,ptr_t));}\
}

#define B_WRAP_SET_S(class,type,list,name) WRAP(type,_set ## name)\
{int32 index; \
 if (argc != 2) ERR_COUNT(type,name); \
 if (!TOINT(0,index)) ERR_TYPE(type,name,1,string); \
 if (!ISSTR(1)) ERR_TYPE(type,name,1,string); \
 GETOBJ(class,type,t); \
 if (t->list[index] == NULL) RETBOOL(false);\
 t->list[index]->name= TStr(WSTR(1));\
 RETBOOL(true);\
}

#define S_WRAP_GET(class,type,list,name) WRAP(type,_get ## name)\
{int32 index; \
 if (argc != 1) ERR_COUNT(type,name); \
 if (!TOINT(0,index)) ERR_TYPE(type,name,1,string); \
 GETOBJ(class,type,t); \
 if (t->list[index] == NULL) RETSTRW(L"");\
 RETSTRWC(t->list[index]->name);\
}

#define B_WRAP_SET_I(class,type,list,name) WRAP(type,_set ## name)\
{int32 index; \
 if (argc != 2) ERR_COUNT(type,name); \
 if (!TOINT(0,index)) ERR_TYPE(type,name,1,string); \
 if (!ISINT(1)) ERR_TYPE(type,name,1,integer); \
 GETOBJ(class,type,t); \
 if (t->list[index] == NULL) RETBOOL(false);\
 t->list[index]->name = INT(1);\
 RETBOOL(true);\
}

#define I_WRAP_GET(class,type,list,name) WRAP(type,_get ## name)\
{int32 index; \
 if (argc != 1) ERR_COUNT(type,name); \
 if (!TOINT(0,index)) ERR_TYPE(type,name,1,string); \
 GETOBJ(class,type,t); \
 if (t->list[index] == NULL) RETBOOL(false);\
 RETINT(t-> list [index]-> name);\
}

#define B_WRAP_S_F(class,type,name)WRAP(type,name)\
{ \
 if (argc == 0 || argc > 2) ERR_COUNT(type,name); \
 if (!ISSTR(0)) ERR_TYPE(type,name,1,string); \
 Stream* out = 0; \
 if (argc == 2) GETFILE(1,out); \
 GETOBJ(class,type,t); \
 RETBOOL(t->name(STR(0),out)); \
}

#define B_WRAP_IR(class,type,name) WRAP(type,name)\
{ \
 if (argc != 2) ERR_COUNT(type,name); \
 int32 v0; TNameValueList * r1; \
 if (!TOINT(0,v0)) ERR_TYPE(type,name,1,integer); \
 TPointer<TParameterList> r1AutoDelete; \
 GETREC(1,r1); if (!r1) ERR_TYPE(type,name,2,record); \
 GETOBJ(class,type,t);\
 RETBOOL(t->name(v0,*r1)); \
}

#define C_WRAP_S(class,type,name) WRAP(type,name)\
{ \
 if (argc != 1) ERR_COUNT(type,name); \
 if (!ISSTR(0)) ERR_TYPE(type,name,1,string);\
 jschar s[2]; s[1]=0; \
 GETOBJ(class,type,t);\
 s[0] = t->name(STR(0)); \
 RETSTRW(s); \
}

#define C_WRAP_V(class,type,name)WRAP(type,name)\
{ \
 if (argc != 0) ERR_COUNT(type,name); \
 jschar s[2]; s[1]=0; \
 GETOBJ(class,type,t);\
 s[0] = t->name(); \
 RETSTRW(s); \
}

#define V_WRAP_C(class,type,name) WRAP(type,name)\
{ \
 if (argc != 1) ERR_COUNT(type,name); \
 GETOBJ(class,type,t);\
 if (!ISSTR(0)) ERR_TYPE(type,name,1,string); \
 t->name((STR(0))[0]); \
 RETBOOL(true);\
}

#define I_WRAP_R(class,type,name) WRAP(type,name)\
{ \
 if (argc != 1) ERR_COUNT(type,name); \
 TNameValueList * r1; \
 TPointer<TParameterList> r1AutoDelete; \
 GETREC(0,r1); if (!r1) ERR_TYPE(type,name,1,record); \
 GETOBJ(class,type,t);\
 RETINT(t->name(*r1)); \
}

#define I_WRAP_R1(class,type,name) WRAP(type,name)\
{ \
 if (argc != 1) ERR_COUNT(type,name); \
 TNameValueList * r1; \
 GETREC1(0,r1); if (!r1) ERR_TYPE(type,name,1,record); \
 GETOBJ(class,type,t);\
 RETINT(t->name(*r1)); \
}

#define I_WRAP_F(class,type,name) WRAP(type,name)\
{ \
 if (argc != 1) ERR_COUNT(type,name); \
 Stream * r1; \
 GETFILE(0,r1); \
 if (!r1) ERR_TYPE(type,name,1,stream); \
 GETOBJ(class,type,t);\
 RETINT(t->name(*r1)); \
}

#define B_WRAP_IS(class,type,name) WRAP(type,name)\
{ \
 if (argc != 2) ERR_COUNT(type,name); \
 int32 v0;\
 if (!TOINT(0,v0)) ERR_TYPE(type,name,1,integer); \
 if (!ISSTR(1)) ERR_TYPE(type,name,2,string); \
 GETOBJ(class,type,t); \
 RETBOOL(t->name(v0,STR(1))); \
}

#define I_WRAP_SS(class,type,name) WRAP(type,name)\
{ \
 if (argc != 2) ERR_COUNT(type,name); \
 int32 v1;\
 if (!ISSTR(0)) ERR_TYPE(type,name,1,string); \
 if (!ISSTR(1)) ERR_TYPE(type,name,2,string); \
 GETOBJ(class,type,t); \
 RETINT(t->name(STR(0),STR(1))); \
}

#define V_WRAP_SS(class,type,name)WRAP(type,name)\
{ \
 if (argc != 2) ERR_COUNT(type,name); \
 int32 v1;\
 if (!ISSTR(0)) ERR_TYPE(type,name,1,string); \
 if (!ISSTR(1)) ERR_TYPE(type,name,2,string); \
 GETOBJ(class,type,t); \
 t->name(STR(0),STR(1)); \
 RETBOOL(true); \
}

#define V_WRAP_II(class,type,name) WRAP(type,name)\
{ \
 if (argc != 2) ERR_COUNT(type,name); \
 int32 v1;\
 if (!ISINT(0)) ERR_TYPE(type,name,1,integer); \
 if (!ISINT(1)) ERR_TYPE(type,name,2,integer); \
 GETOBJ(class,type,t); \
 t->name(INT(0),INT(1)); \
 RETBOOL(true); \
}

#define V_WRAP_I(class,type,name) WRAP(type,name)\
{ \
 if (argc != 1) ERR_COUNT(type,name); \
 int32 v1;\
 if (!ISINT(0)) ERR_TYPE(type,name,1,integer); \
 GETOBJ(class,type,t); \
 t->name(INT(0)); \
 RETBOOL(true); \
}

#define V_WRAP_S(class,type,name) WRAP(type,name)\
{ \
 if (argc != 1) ERR_COUNT(type,name); \
 int32 v1;\
 if (!ISSTR(0)) ERR_TYPE(type,name,1,string); \
 GETOBJ(class,type,t); \
 t->name(STR(0)); \
 RETBOOL(true); \
}

#define B_WRAP_I(class,type,name) WRAP(type,name)\
{ \
 if (argc == 0) ERR_COUNT(type,name); \
 int32 v1;\
 if (!TOINT(0,v1)) ERR_TYPE(type,name,1,integer); \
 GETOBJ(class,type,t); \
 RETBOOL(t->name(v1)); \
}

#define S_WRAP_I(class,type,name) WRAP(type,name)\
{ \
 if (argc == 0)  ERR_COUNT(type,name); \
 int32 v1; if (!TOINT(0,v1)) ERR_TYPE(type,name,1,integer); \
 GETOBJ(class,type,t); \
 RETSTRW(WStr(t->name(v1))); \
}


#define S_WRAP_II(class,type,name) WRAP(type,name)\
{ \
 if (argc == 0)  ERR_COUNT(type,name); \
 int32 v1; if (!TOINT(0,v1)) ERR_TYPE(type,name,1,integer); \
 int32 v2; if (!TOINT(0,v2)) ERR_TYPE(type,name,2,integer); \
 GETOBJ(class,type,t); \
 RETSTRW(TStr(t->name(v1,v2))); \
}

#if 0
#define S_WRAP_II(class,type,name) \
static JSBool type ## _ ## name(JSContext *cx, \
 JSObject *obj, uintN argc, jsval *argv, jsval *rval) \
{ \
 if (argc == 0)  ERR_COUNT(type,name); \
 int32 v1; if (!TOINT(0,v1)) ERR_TYPE(type,name,1,integer); \
 int32 v2; if (!TOINT(0,v2)) ERR_TYPE(type,name,2,integer); \
 GETOBJ(class,type,t); \
 RETSTR(t->name(v1,v2)); \
}
#endif


#define I_WRAP_I(class,type,name) WRAP(type,name)\
{ \
 if (argc == 0) ERR_COUNT(type,name); \
 int32 v1; if (!TOINT(0,v1)) ERR_TYPE(type,name,1,integer); \
 GETOBJ(class,type,t); \
 RETINT(t->name(v1)); \
}


#define I_WRAP_V(class,type,name) WRAP(type,name)\
{ \
 if (argc != 0) ERR_COUNT(type,name); \
 GETOBJ(class,type,t); \
 RETINT(t->name()); \
}

#if 0 //never used
#define S_WRAP_V(class,type,name) WRAP(type,name)\
{ \
 GETOBJ(class,type,t); \
 RETSTRW(TStr(t->name())); \
}

#define B_WRAP_V(class,type,name) WRAP(type,name)\
{ \
 GETOBJ(class,type,t); \
 RETBOOL(t->name()); \
}
#endif

#define V_WRAP_V(class,type,name) WRAP(type,name)\
{ \
 GETOBJ(class,type,t); \
 t->name(); \
 RETBOOL(true); \
}

#define WRAP_HELP(class,text) WRAP(class,HELP)\
{ RETSTRW(text); }


//#define DEBUG
//#define TRACE
//dword-word-word-byte[8]
//{85BBD920-42A0-1069-A2E4-08002B30309D}
//HRESULT CLSIDFromString(LPOLESTR lpsz,LPCLSID pclsid);
//S_OK == StringFromCLSID(REFCLSID rclsid,LPOLESTR * ppsz);

#ifndef DISPID_PROPERTYPUT
#   define DISPID_PROPERTYPUT (-3)
#endif


JSObject*
ActiveX_Object( JSContext *cx, ActiveX* t );

namespace
{

//using namespace mozjs;

JSClassOps jsOps = {
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    mozjs::JsFinalizeOp<ActiveX>,
    nullptr,
    nullptr,
    nullptr,
    nullptr
};

JSClass ActiveX_Class = {
    "ActiveX",
    JSCLASS_HAS_PRIVATE| JSCLASS_BACKGROUND_FINALIZE,
    &jsOps
};


//shadow property 0 to 255
bool
ActiveX_JSGet_Impl( JSContext* cx, unsigned argc, JS::Value* vp )
{// JSContext *cx, JSObject *obj, jsid id, jsval *rval 
    /*
    if ( !JSVAL_IS_INT( id ) ) return false;
    int x = JSVAL_TO_INT( id );
    */

    JS::CallArgs args = JS::CallArgsFromVp( argc, vp );

    if ( argc < 1 )
    {
        JS_ReportErrorASCII( cx, "ActiveX_JSGet error" );
        return false;
    }
    if ( !args[0].isString() )
    {
        JS_ReportErrorASCII( cx, "ActiveX_JSGet error: argument 1 is not a string" );
        return false;
    }

    std::wstring name = mozjs::JsToNative<std::wstring>::Convert( cx, args[0] );
    
    /*
    GETENV;
    GETOBJ2( ActiveX, ActiveX, t );*/
    ActiveX* t = static_cast<ActiveX*>(JS_GetPrivate( args.thisv().toObjectOrNull() ));
    if ( !t )
    {
        //JS_ReportErrorASCII( cx, "Internal error: JS_GetPrivate failed" );
        return false;
    }

    /*
    Default

    switch ( x )
    {
    case 255: 
    {
        JS::RootedString str (JS_NewUCStringCopyZ( cx, (char16_t*)L"ActiveX" ));
        if ( !str )
        {
            return false;
        }
        args.rval().setString( str );
        return true;
    }
    }
    */

    ActiveX::PropInfo * p = t->Find(name);
    DISPID dispid = 0;
    if ( p && t->Id( p->name, dispid ) )
    {
        return t->Get( dispid, cx, 0, vp );
    }

    return false;
}

bool
ActiveX_JSSet_Impl( JSContext *cx, unsigned argc, JS::Value* vp )
{
    /*
    if ( !JSVAL_IS_INT( id ) ) return false;
    int x = JSVAL_TO_INT( id );
    */

    JS::CallArgs args = JS::CallArgsFromVp( argc, vp );
    /*
    GETENV;
    GETOBJ2( ActiveX, ActiveX, t );
    */

    JS::RootedString s( cx, JS_GetFunctionId( JS_ValueToFunction( cx, args.calleev() ) ) );
    if ( !s )
    {
        JS_ReportErrorASCII( cx, "ActiveX_Exec error: No property name" );
        return false;
    }

    size_t strLen = JS_GetStringLength( s );
    std::wstring name( strLen + 1, '\0' );
    mozilla::Range<char16_t> wCharStr( (char16_t*)name.data(), strLen );
    if ( !JS_CopyStringChars( cx, wCharStr, s ) )
    {
        JS_ReportOutOfMemory( cx );
        return false;
    }

    std::size_t fPos = name.find( L" " );
    if ( fPos == std::string::npos )
    {
        std::string tmpStr = pfc::stringcvt::string_utf8_from_wide( name.c_str() );
        JS_ReportErrorASCII( cx, "ActiveX_JSSet error: invalid command: %s", tmpStr.c_str() );
        return false;
    }
    name = std::wstring( name.begin() + fPos + 1, name.end() );

    ActiveX* t = static_cast<ActiveX*>(JS_GetPrivate( args.thisv().toObjectOrNull() ));
    if ( !t )
    {
        //JS_ReportErrorASCII( cx, "Internal error: JS_GetPrivate failed" );
        return false;
    }

    // DISPID dispid=0;
    // if (t->Id(id,dispid))

    ActiveX::PropInfo * p = t->Find( name );
    DISPID dispid = 0;
    if ( p && t->Id( p->name, dispid ) )
    {
        return t->Set( dispid, cx, 0, vp, p->PutRef );
    }

    return false;
}

/*
const JSPropertySpec ActiveX_properties[] = {
    JS_PSG( "className", ActiveX_JSGet, JSPROP_ENUMERATE | JSPROP_READONLY | JSPROP_PERMANENT ),
    JS_PS_END
};
*/

/*
const JSFunctionSpec ActiveX_functions[] = {
    JS_FN( "get", ActiveX_Get, 1, 0 ),
    JS_FN( "set", ActiveX_Set, 2, 0 ),
    JS_FN( "exec", ActiveX_Exec, 2, 0 ),
    JS_FN( "at", ActiveX_Exec, 2, 0 ),
    JS_FN( "as", ActiveX_as, 2, 0 ),
    JS_FN( "close", ActiveX_Close, 0, 0 ),
    JS_FN( "toString", ActiveX_ToString, 0, 0 ),
    JS_FS_END
};
*/

/*
static JSFunctionSpec ActiveX_functions[] = {
{ "get",     ActiveX_Get,      1 },
{ "set",    ActiveX_Set, 2 },
{ "exec",    ActiveX_Exec, 2 },
{ "at",    ActiveX_Exec, 2 },
{ "as",    ActiveX_as, 2 },
{ "close",ActiveX_Close,0 },
{ "toString",ActiveX_ToString,0 },
{ 0 }
};
*/
/*
WRAP_HELP( ActiveX,
           "name(index)\nextract(index)\nextract(index,string)\nsize(index)\n"
           "close()\n" );
*/


bool ActiveX_Run_Impl( JSContext* cx, unsigned argc, JS::Value* vp )
{
    /*
    GETENV;
    GETARGS;
    GETOBJ( ActiveX, ActiveX, t );
    */

    JS::CallArgs args = JS::CallArgsFromVp( argc, vp );

    ActiveX* t = static_cast<ActiveX*>(JS_GetPrivate( args.thisv().toObjectOrNull() ));
    if ( !t )
    {
        //JS_ReportErrorASCII( cx, "Internal error: JS_GetPrivate failed" );
        return false;
    }
    
    JS::RootedString s (cx, JS_GetFunctionId( JS_ValueToFunction( cx, args.calleev() ) ));
    if ( !s )
    {
        JS_ReportErrorASCII( cx, "ActiveX_Exec error: No function name" );
        return false;
    }

    size_t strLen = JS_GetStringLength( s );
    std::wstring name( strLen + 1, '\0' );
    mozilla::Range<char16_t> wCharStr( (char16_t*)name.data(), strLen );
    if ( !JS_CopyStringChars( cx, wCharStr, s ) )
    {
        JS_ReportOutOfMemory( cx );
        return false;
    }
    
    DISPID dispid;
    if ( !t->Id( name, dispid ) )
    {
        std::string tmpStr = pfc::stringcvt::string_utf8_from_wide( name.c_str() );
        JS_ReportErrorASCII( cx, "ActiveX error: This object does not have that function: %s", tmpStr.c_str() );
        return false;
    }
    if ( !t->Invoke( dispid, cx, argc, vp ) )
    {
        std::string tmpStr = pfc::stringcvt::string_utf8_from_wide( name.c_str() );
        JS_ReportErrorASCII( cx, "ActiveX error: IDispatch->Invoke failed: %s", tmpStr.c_str() );
        return false;
        //      RETOBJ(0);
                //JavaScript handles the exception with SetPendingException
    }
    return true;
}

MJS_WRAP_JS_TO_NATIVE_FN( ActiveX_JSGet, ActiveX_JSGet_Impl )
MJS_WRAP_JS_TO_NATIVE_FN( ActiveX_JSSet, ActiveX_JSSet_Impl )
MJS_WRAP_JS_TO_NATIVE_FN( ActiveX_Run, ActiveX_Run_Impl )

// bool ActiveX_Exec( JSContext* cx, unsigned argc, JS::Value* vp )
// {
//     /*
//     GETENV;
//     GETOBJ( ActiveX, ActiveX, t );
//     */
// 
//     JS::CallArgs args = JS::CallArgsFromVp( argc, vp );
// 
//     ActiveX* t = static_cast<ActiveX*>(JS_GetPrivate( args.thisv().toObjectOrNull() ));
//     if ( !t )
//     {
//         //JS_ReportErrorASCII( cx, "Internal error: JS_GetPrivate failed" );
//         return false;
//     }
// 
//     if ( argc < 1 )
//     {
//         JS_ReportErrorASCII( cx, "ActiveX_Exec error" );
//         return false;
//     }
//     if ( !args[0].isString() )
//     {
//         JS_ReportErrorASCII( cx, "ActiveX_Exec error: argument 1 is not a string" );
//         return false;        
//     }
// 
//     JS::RootedString s( JS_GetFunctionId( ARGV( 0) ) );
//     if ( s )
//     {        
//         const wchar_t* name = (wchar_t*)JS_GetStringChars( s );
//         if ( !name )
//         {
//             JS_ReportErrorASCII( cx, "ActiveX_Exec error: No function name" );
//             args.rval().setUndefined();
//             return false;
//         }
//         DISPID dispid;
//         if ( !t->Id( name, dispid ) )
//         {
//             std::string tmpStr( pfc::stringcvt::string_utf8_from_wide( name ) );
//             JS_ReportErrorASCII( cx, "ActiveX error: This object does not have that function: %s", tmpStr.c_str() );
//             args.rval().setUndefined();
//             return false;            
//         }
//         if ( !t->Invoke( dispid, cx, argc - 1, args + 1, rval ) )
//         {
//             return false;
//             //      ERR_MSG(ActiveX,"IDispatch->Invoke failed",TStr(name));
//         }
//     }
//     return true;
// }
// 
// ///Get("property","index","index")
// bool ActiveX_ToString( JSContext* cx, unsigned argc, JS::Value* vp )
// {
//     /*
//     GETARGS;
//     GETENV;
//     GETOBJ( ActiveX, ActiveX, t );
//     */
// 
//     JS::CallArgs args = JS::CallArgsFromVp( argc, vp );
// 
//     // TODO: don't actually need this, move to class
//     ActiveX* t = static_cast<ActiveX*>( JS_GetPrivate( args.thisv().toObjectOrNull() ) );    
//     if ( !t )
//     {
//         return false;
//     }
// 
//     if ( t->variant.vt )
//     {
//         JS::RootedString jsString( JS_NewStringCopyZ( cx, "variant" ) );
//         if ( !jsString )
//         {
//             return false;
//         }
// 
//         args.rval().setString( jsString );
//         return true;
//     }
// 
//     DISPID dispid = 0;
//     if ( t->Id( (wchar_t*)L"toString", dispid ) )
//     {
//         t->Invoke( dispid, cx, argc, vp );
//         return true;
//     }
// 
//     //if (!t->Get(dispid, cx, 0,vp, false))
// 
//     JS::RootedString jsString( JS_NewStringCopyZ( cx, "" ) );
//     if ( !jsString )
//     {
//         return false;
//     }
// 
//     args.rval().setString( jsString );
//     return true;
// }
// 
// bool ActiveX_Get( JSContext* cx, unsigned argc, JS::Value* vp )
// {
//     if ( argc == 0 ) ERR_COUNT( ActiveX, Get );
//     if ( !ISSTR( 0 ) && !ISINT( 0 ) ) ERR_TYPE( ActiveX, Get, 1, String );
// 
//     /*GETENV;
//     GETOBJ( ActiveX, ActiveX, t );*/
// 
//     JS::CallArgs args = JS::CallArgsFromVp( argc, vp );
// 
//     ActiveX* t = static_cast<ActiveX*>(JS_GetPrivate( args.thisv().toObjectOrNull() ));
//     if ( !t )
//     {
//         //JS_ReportErrorASCII( cx, "Internal error: JS_GetPrivate failed" );
//         return false;
//     }
// 
// 
//     DISPID dispid = 0;
//     if ( ISSTR( 0 ) )
//     {
//         JSString* s = JSVAL_TO_STRING( args[0] );
// 
//         if ( s )
//         {
//             jschar* name = JS_GetStringChars( s );
//             if ( !name )
//             {
//                 JS_ReportErrorASCII( cx, "ActiveX_Exec error: No property name" );
//                 args.rval().setUndefined();
//                 return false;
//             }
//             if ( !t->Id( name, dispid ) )
//             {
//                 std::string tmpStr( pfc::stringcvt::string_utf8_from_wide( name ) );
//                 JS_ReportErrorASCII( cx, "ActiveX error: This object does not have that property: %s", tmpStr.c_str() );
//                 args.rval().setUndefined();
//                 return false;
//             }
//         }
//     }
// 
//     if ( !t->Get( dispid, cx, argc - 1, argv + 1, rval ) )
//     {
//         //    ERR_MSG(ActiveX,"IDispatch->Invoke failed","");
//         return false;
//     }
//     return true;
// }
// 
// ///Set("property","index","index","value")
// bool ActiveX_Set( JSContext* cx, unsigned argc, JS::Value* vp )
// {
//     if ( argc < 2 ) ERR_COUNT( ActiveX, Set );
//     if ( !ISSTR( 0 ) ) ERR_TYPE( ActiveX, Set, 1, String );
// 
//     /*GETENV;
//     GETOBJ2( ActiveX, ActiveX, t );*/
// 
//     JS::CallArgs args = JS::CallArgsFromVp( argc, vp );
// 
//     ActiveX* t = static_cast<ActiveX*>(JS_GetPrivate( args.thisv().toObjectOrNull() ));
//     if ( !t )
//     {
//         //JS_ReportErrorASCII( cx, "Internal error: JS_GetPrivate failed" );
//         return false;
//     }
// 
//     JSString* s = JSVAL_TO_STRING( args[0] );
//     if ( s )
//     {
//         jschar* name = JS_GetStringChars( s );
//         if ( !name )
//         {
//             JS_ReportErrorASCII( cx, "ActiveX_Exec error: No property name" );
//             args.rval().setUndefined();
//             return false;
//         }
// 
//         DISPID dispid;
//         if ( !t->Id( name, dispid ) )
//         {
//             std::string tmpStr( pfc::stringcvt::string_utf8_from_wide( name ) );
//             JS_ReportErrorASCII( cx, "ActiveX error: This object does not have that property: %s", tmpStr.c_str() );
//             args.rval().setUndefined();
//             return false;
//         }
// 
//         ActiveX::PropInfo *p = t->Find( name );
//         RETBOOL( t->Set( dispid, cx, argc - 2, argv + 1, argv + argc - 1, p ? p->PutRef : false ) );
//     }
// 
//     args.rval().setUndefined();
//     return true;
// }
// 
// bool ActiveX_as( JSContext* cx, unsigned argc, JS::Value* vp )
// {
//     if ( argc < 1 ) ERR_COUNT( ActiveX, as );
//     if ( !ISSTR( 0 ) ) ERR_TYPE( ActiveX, as, 1, String );
//     GETENV;
//     GETOBJ( ActiveX, ActiveX, t );
// 
//     HRESULT hresult;
//     void* specific = nullptr;
//     CLSID clsid;
//     jschar * type = JS_GetStringChars( JSVAL_TO_STRING( argv[0] ) );
// 
//     if ( type[0] == L'{' )
//         hresult = CLSIDFromString( (WCHAR*)type, &clsid );
//     else
//         hresult = CLSIDFromProgID( (WCHAR*)type, &clsid );
// 
//     if ( SUCCEEDED( hresult ) )
//     {
//         IUnknown * unk;
//         hresult = t->unknown->QueryInterface( clsid, (void * *)&unk );
//         if ( SUCCEEDED( hresult ) ) RETOBJ( ActiveX_Object( cx, new ActiveX( unk ), true, nullptr ) );
// 
//     }
//     RETOBJ( nullptr );
// }
// 
// bool ActiveX_Close( JSContext* cx, unsigned argc, JS::Value* vp )
// {    
//     CLOSEPRIVATE( ActiveX, ActiveX );
//     RETBOOL( true );
// }
// 
/*
static JSFunctionSpec ActiveX_fnstatic[] = {
    { "help",  ActiveX_HELP,    0 },
#ifdef EXPERIMENT_COM
{ "typelib",  ActiveX_typelib,    1 },
#endif
{ 0 }
};
*/

}

#define FETCH(x) (ref? * (var.p ## x) : var.x)

///RetrieveValue assumes that the caller will call VariantClear, so call AddRef on new objects
bool RetrieveValue( VARIANTARG& var, JSContext* cx, JS::MutableHandleValue rval )
{
    /*
    ENTERNATIVE( cx );
    */

    bool ref = false;
    int type = var.vt;
    if ( type & VT_BYREF ) 
    { 
        ref = true; 
        type &= ~VT_BYREF; 
    }

    try
    {
        switch ( type )
        {
        case VT_ERROR: rval.setUndefined(); break;
        case VT_NULL:
        case VT_EMPTY: rval.setNull(); break;
        case VT_I1: rval.setInt32( static_cast<int32_t>(FETCH( cVal )) ); break;
        case VT_I2: rval.setInt32( static_cast<int32_t>(FETCH( iVal )) ); break;
        case VT_INT:
        case VT_I4: rval.setInt32( FETCH( lVal ) ); break;
        case VT_R4: rval.setNumber( FETCH( fltVal ) ); break;
        case VT_R8: rval.setNumber( FETCH( dblVal ) ); break;

        case VT_BOOL: rval.setBoolean( FETCH( boolVal ) ? true : false ); break;

        case VT_UI1: rval.setNumber( static_cast<uint32_t>(FETCH( bVal )) ); break;
        case VT_UI2: rval.setNumber( static_cast<uint32_t>(FETCH( uiVal )) ); break;
        case VT_UINT:
        case VT_UI4: rval.setNumber( static_cast<uint32_t>(FETCH( ulVal )) ); break;

        case VT_BSTR:
        {
            JS::RootedString rstr( cx, JS_NewUCStringCopyN( cx, (char16_t*) FETCH( bstrVal ), SysStringLen( FETCH( bstrVal ) ) ) );
            rval.setString( rstr );
            break;
            //              SysFreeString(FETCH(bstrVal));
            //              var.vt = VT_EMPTY;
        };
        case VT_DATE:
        {
            DATE d = FETCH( date );
            SYSTEMTIME time;
            VariantTimeToSystemTime( d, &time );
            rval.setObjectOrNull( ROOT( JS_NewDateObject( cx, time.wYear, time.wMonth - 1, time.wDay,
                                                             time.wHour, time.wMinute, time.wSecond ) ) );

            break;
        }

        case VT_UNKNOWN:
        {
            if ( !FETCH( punkVal ) ) { rval.setNull(); break; }
            ActiveX* x = new ActiveX( FETCH( punkVal ), true );
            if ( !x->unknown && !x->dispatch ) { delete x; return false; }
            rval.setObjectOrNull( ActiveX_Object( cx, x ) );
            break;
        }
        case VT_DISPATCH:
        {
            if ( !FETCH( pdispVal ) ) { rval.setNull(); break; }
            ActiveX* x = new ActiveX( FETCH( pdispVal ), true );
            if ( !x->unknown && !x->dispatch ) { delete x; return false; }
            rval.setObjectOrNull( ActiveX_Object( cx, x ) );
            break;
        }
        case VT_VARIANT: //traverse the indirection list?
            if ( ref )
            {
                VARIANTARG* v = var.pvarVal;
                if ( v )
                    return RetrieveValue( *v, cx, rval );
            }
            break;

        default:
            if ( type <= VT_CLSID )
            {
                ActiveX* x = new ActiveX( var );
                rval.setObjectOrNull( ActiveX_Object( cx, x ) );

                return true;
            }

            return false;
            //default: return false;
        }
    }
    catch ( ... )
    {
        return false;
    }
    return true;
}


void CheckReturn( JSContext* cx, JS::MutableHandleValue rval )
{
    if ( rval.isObject() )
    {
        HRESULT hresult;
        JS::RootedObject j0 (cx, rval.toObjectOrNull());
        if ( j0 && JS_InstanceOf( cx, j0, &ActiveX_Class, 0 ) )
        {
            ActiveX* x = static_cast<ActiveX*>( JS_GetPrivate( j0 ) );            
            if ( x->unknown && !x->dispatch )
            {
                hresult = x->unknown->QueryInterface( IID_IDispatch, (void **)&x->dispatch );
                if ( SUCCEEDED( hresult ) )
                {
                    x->SetupMembers( cx, j0 );
                }
                else
                {
                    x->dispatch = 0;
                }
            }
        }
    }
}

bool SetupValue( VARIANTARG& arg, JSContext* cx, JS::MutableHandleValue rval )
{
    VariantInit( &arg );
    // arg.vt = VT_EMPTY;
    
    if ( rval.isObject() )
    {        
        JS::RootedObject j0( cx, rval.toObjectOrNull() );
        if ( j0 && JS_InstanceOf( cx, j0, &ActiveX_Class, 0 ) )
        {
            ActiveX* x = static_cast<ActiveX*>(JS_GetPrivate( j0 ));
            if ( !x )
            {
                //JS_ReportErrorASCII( cx, "Internal error: JS_GetPrivate failed" );
                return false;
            }
            if ( x->variant.vt != VT_EMPTY )
            {
                //1.7.2.3
                VariantCopyInd( &arg, &x->variant );
                //VariantCopy(&arg,&x->variant);
                //1.7.2.2 could address invalid memory if x is freed before arg
                // arg.vt = VT_VARIANT | VT_BYREF;
                // arg.pvarVal = &x->variant;
                return true;
            }
            if ( x->dispatch )
            {
                arg.vt = VT_DISPATCH;
                arg.pdispVal = x->dispatch;
                x->dispatch->AddRef();
                return true;
            }
            else if ( x->unknown )
            {
                arg.vt = VT_UNKNOWN;
                arg.punkVal = x->unknown;
                x->unknown->AddRef();
                return true;
            }
            else
            {
                arg.vt = VT_BYREF | VT_UNKNOWN;
                arg.ppunkVal = &x->unknown;
                return true;
            }
        }
    }
    else if ( rval.isBoolean() )
    {
        arg.vt = VT_BOOL;
        arg.boolVal = rval.toBoolean() ? -1 : 0;
        return true;
    }
    else if ( rval.isInt32() )
    {
        arg.vt = VT_I4;
        arg.lVal = rval.toInt32();
        return true;
    }
    else if ( rval.isDouble() )
    {
        arg.vt = VT_R8;
        arg.dblVal = rval.toDouble();

        return true;
    }
    else if ( rval.isNull() )
    {
        arg.vt = VT_EMPTY;
        arg.scode = 0;
        return true;
    }
    else if ( rval.isString() )
    {
        JS::RootedString rStr(cx, rval.toString() );
        size_t strLen = JS_GetStringLength( rStr );
        std::wstring strVal( strLen + 1, '\0' );
        mozilla::Range<char16_t> wCharStr( (char16_t*)strVal.data(), strLen );
        if ( !JS_CopyStringChars( cx, wCharStr, rStr ) )
        {
            JS_ReportOutOfMemory( cx );
            return false;
        }

        arg.vt = VT_BSTR;
        arg.bstrVal = SysAllocString( strVal.c_str() );
        return true;
    }    

    return false;
}

#ifdef EXPERIMENT_COM
IDispatch* Recast( IUnknown* unk, ITypeLib* typelib, WCHAR* type )
{
    IUnknown* result = nullptr;
    IDispatch* dispatch = nullptr;
    ITypeInfo* typeinfo = nullptr;
    void* specific = nullptr;
    HRESULT hresult;
    unsigned short found = 1;
    MEMBERID memb = 0;
    CLSID clsid;

    if ( type[0] == L'{' )
        hresult = CLSIDFromString( (WCHAR*)type, &clsid );
    else
        hresult = CLSIDFromProgID( (WCHAR*)type, &clsid );
    if ( !SUCCEEDED( hresult ) )
        return nullptr;

    hresult = typelib->GetTypeInfoOfGuid( clsid, &typeinfo );
    if ( !SUCCEEDED( hresult ) || !found )
        return nullptr;

    hresult = unk->QueryInterface( clsid, &specific );
    if ( !SUCCEEDED( hresult ) || !specific )
        return nullptr;

    hresult = CreateStdDispatch( unk, specific, typeinfo, &result );

    if ( !SUCCEEDED( hresult ) || !result )
        return nullptr;

    hresult = result->QueryInterface( IID_IDispatch, (void * *)&dispatch );

    if ( !SUCCEEDED( hresult ) )
        dispatch = 0;

    return dispatch;
}

WRAP( ActiveX, typelib )
{
    if ( argc < 1 ) ERR_COUNT( ActiveX, typelib );
    const jschar * library = WSTR( 0 );
    ITypeLib * typelib = nullptr;

    HRESULT hresult = LoadTypeLib( (WCHAR*)library, &typelib );
    if ( SUCCEEDED( hresult ) )
    {
        //     MemoryStream s;
        WStr s;

        unsigned count = typelib->GetTypeInfoCount();
        for ( unsigned i = 0; i < count; i++ )
        {
            BSTR name, docstring, helpfile;
            unsigned long helpcontext;

            typelib->GetDocumentation( i, &name, &docstring, &helpcontext, &helpfile );
            WStr w1( (wchar_t*)name, SysStringLen( name ) );
            WStr w2( (wchar_t*)docstring, SysStringLen( docstring ) );

            s << (w1) << (wchar_t*)L": " << (w2) << (wchar_t*)L"\n";

            SysFreeString( name );
            SysFreeString( docstring );
            SysFreeString( helpfile );
        }

        RETSTRW( s );
    }
    RETSTRW( L"" );
}

WRAP( ActiveX, as )
{
    if ( argc < 2 ) ERR_COUNT( ActiveX, as );
    if ( !ISSTR( 0 ) || !ISSTR( 1 ) ) ERR_TYPE( ActiveX, as, 1, String );
    GETENV;
    GETOBJ( ActiveX, ActiveX, t );

    jschar * library = JS_GetStringChars( JSVAL_TO_STRING( argv[0] ) );
    jschar * name = JS_GetStringChars( JSVAL_TO_STRING( argv[1] ) );
    ITypeLib * typelib = nullptr;

    HRESULT hresult = LoadTypeLib( (WCHAR*)library, &typelib );
    if ( SUCCEEDED( hresult ) )
    {
        IDispatch* d = Recast( t->unknown, typelib, (WCHAR*)name );
        if ( d )
        {
            if ( t->dispatch ) t->dispatch->Release();
            t->dispatch = d;
            t->SetupMembers( cx, obj );
        }

        typelib->Release();

        if ( d )  RETOBJ( obj );
    }
    RETOBJ( nullptr );
}
#endif

/*
char * DeflateString(const jschar*chars, size_t length)
{
    size_t i, size;
    char *bytes;

    size = (length + 1) * sizeof(char);
    bytes = (char *) malloc(size);
    if (!bytes) return nullptr;
    for (i = 0; i < length; i++)
        bytes[i] = (char) chars[i];
    bytes[i] = 0;
    return bytes;
}
*/
void ReportActiveXError( HRESULT hresult, EXCEPINFO& exception, UINT& argerr, JSContext* cx )
{
    std::string errMsg = "ActiveX:";
    //#define ReportError1(msg,arg)
    // sprintf(errmsg,"ActiveX:  msg,arg);
    // JS_ReportError(cx,STRING_TO_JSVAL(JS_NewStringCopyZ(cx,errmsg )));

#define ReportError1(msg,arg) \
 {errMsg += msg; \
  JS_ReportErrorASCII(cx,errMsg.c_str(), arg);}

#define ReportError(msg) \
 {errMsg += msg; \
  JS_ReportErrorASCII(cx,errMsg.c_str());}

//  #define ReportError1(msg,arg) JS_ReportError(cx,"ActiveX: " msg,arg);
//#define ReportError(msg) JS_ReportError(cx,"ActiveX: " msg);
// JS_SetPendingException(cx,STRING_TO_JSVAL(JS_NewStringCopyZ(cx,errmsg )));

    switch ( hresult )
    {
    case DISP_E_BADPARAMCOUNT: ReportError( "Wrong number of parameters" ); break;
    case DISP_E_BADVARTYPE: ReportError1( "Bad variable type %d", argerr ); break;
    case DISP_E_EXCEPTION:
    {
        if ( exception.bstrDescription )
        {
            std::wstring w1( (wchar_t*)exception.bstrDescription, SysStringLen( exception.bstrDescription ) );
            //                          TStr d(w1);

            std::wstring w2( (wchar_t*)exception.bstrSource, SysStringLen( exception.bstrSource ) );
            //                          TStr s(w2);
            std::wstring w;
            w.resize( w1.length() + w2.length() + 20 );
            swprintf( w.data(), w.size(), L"ActiveX: (%s) %s", w2.c_str(), w1.c_str() );
            //                          TStr err(w);
            //                          printf("%s\n",(char*)err);

            // <codecvt> is deprecated in C++17...
            std::string tmpStr( pfc::stringcvt::string_utf8_from_wide( w.c_str() ) );

            JS_ReportErrorASCII( cx, tmpStr.c_str() );
            //                          char* err = DeflateString((jschar*)exception.bstrDescription,SysStringLen(exception.bstrDescription));
            //                          ReportError(err);
            //                          ReportError(cx,"%s",TStr("Activex: ",err));
            //                          JS_ReportError(cx,"%s",(char*)TStr(err));
            //                          free(err);
        }
        else
        {
            ReportError1( "Error code %d", exception.scode );
        }
        SysFreeString( exception.bstrSource );
        SysFreeString( exception.bstrDescription );
        SysFreeString( exception.bstrHelpFile );
        break;
    }
    case DISP_E_MEMBERNOTFOUND: ReportError( "Function not found" ); break;
    case DISP_E_OVERFLOW: ReportError1( "Can not convert variable %d", argerr ); break;
    case DISP_E_PARAMNOTFOUND: ReportError1( "Parameter %d not found", argerr ); break;
    case DISP_E_TYPEMISMATCH: ReportError1( "Parameter %d type mismatch", argerr ); break;
    case DISP_E_UNKNOWNINTERFACE: ReportError( "Unknown interface" ); break;
    case DISP_E_UNKNOWNLCID: ReportError( "Unknown LCID" ); break;
    case DISP_E_PARAMNOTOPTIONAL: ReportError1( "Parameter %d is required", argerr );
    }
}

JSObject*
ActiveX_Object( JSContext *cx, ActiveX* t)
{
    /*
    GETENV;
    ENTERNATIVE( cx );
    */

    JS::RootedObject obj( cx, JS_NewObject( cx, &ActiveX_Class ));
    //JS_DefineFunctions( cx, obj, ActiveX_functions );
    //JS_DefineProperties( cx, obj, ActiveX_properties );
    /*obj = JS_NewObject(cx, &ActiveX_Class,Env->ActiveX, nullptr);
    JS_DefineFunctions(cx,obj,ActiveX_functions);
    JS_DefineProperties(cx,obj,ActiveX_properties);   */
    if ( t )
    {        
        JS_SetPrivate( obj, t );
        t->SetupMembers( cx, obj );
    }
    return obj;
}

bool ActiveX_Constructor( JSContext* cx, unsigned argc, JS::Value* vp )
{
    JS::CallArgs args = JS::CallArgsFromVp( argc, vp );

    /*
    GETENV;
    if ( Env->SafeMode )
    {
        JS_ReportErrorASCII( cx, "ActiveX error: blocked by security settings" );
        args.rval().setUndefined();
        return false;
    }
    */

    if ( argc )
    {
        if ( !args[0].isString() )
        {
            JS_ReportErrorASCII( cx, "ActiveX_Exec error: argument 1 is not a string" );
            return false;
        }
    }        
    //ENTERNATIVE(cx);

    //argc > 0 if clsid is valid
    CLSID clsid;
    HRESULT hresult;   
    std::wstring name = argc ? mozjs::JsToNative<std::wstring>::Convert( cx, args[0] ) : std::wstring();

    if ( argc )
    {
        if ( name[0] == L'{' )
            hresult = CLSIDFromString( name.c_str(), &clsid );
        else
            hresult = CLSIDFromProgID( name.c_str(), &clsid );

        if ( !SUCCEEDED( hresult ) )
        {
            std::string tmpStr = pfc::stringcvt::string_utf8_from_wide( name.c_str() );
            JS_ReportErrorASCII( cx, "ActiveX error: invalid CLSID" );
            return false;
        }
    }

    std::unique_ptr<ActiveX> t;

    if ( argc == 0 )
    {
        t.reset( new ActiveX() );
    }
    else
    {
        IUnknown* unk = nullptr;
        if ( argc == 1 )
        {
            hresult = GetActiveObject( clsid, nullptr, &unk );
        }

        if ( SUCCEEDED( hresult ) && unk )
        {
            t.reset( new ActiveX( unk ) );
            if ( !t->unknown )
            {
                std::string cStr = pfc::stringcvt::string_utf8_from_wide( name.c_str() );
                JS_ReportErrorASCII( cx, "ActiveX error: Can't create ActiveX object: %s", cStr.c_str() );
                args.rval().setUndefined();
                return false;
            }
        }
    }

    if ( !t )
    {
        t.reset( new ActiveX( clsid ) );
        if ( !t->unknown )
        {
            std::string cStr = pfc::stringcvt::string_utf8_from_wide( name.c_str() );
            JS_ReportErrorASCII( cx, "ActiveX error: Can't create ActiveX object: %s", cStr.c_str() );
            args.rval().setUndefined();
            return false;
        }
    }
    if ( t )
    {
        JS::RootedObject retObj( cx, ActiveX_Object( cx, t.get() ) );
        if ( !retObj )
        {
            return false;
        }

        args.rval().setObjectOrNull( retObj );
    }

    t.release();
    return true;
}

JSObject* CreateActiveXProto( JSContext *cx, JS::HandleObject obj )
{
    /* INITCLASS( ActiveX ); */
    return JS_InitClass( cx, obj, nullptr, &ActiveX_Class,
                         ActiveX_Constructor, 0,
                         nullptr, nullptr, nullptr, nullptr );
    /*
    Env->oActiveX = JS_InitClass( cx, obj, nullptr, &ActiveX_Class,
                                  ActiveX_Constructor, 0,
                                   ActiveX_properties, ActiveX_functions, nullptr,ActiveX_fnstatic);
    */
}

ActiveX::ActiveX( VARIANTARG& var )
{
    unknown = nullptr;
    typeinfo = nullptr;
    dispatch = nullptr;
    VariantInit( &variant );
    VariantCopyInd( &variant, &var );
}

ActiveX::ActiveX()
{
    unknown = nullptr;
    typeinfo = nullptr;
    dispatch = nullptr;
    memset( &variant, 0, sizeof( variant ) );
}

ActiveX::ActiveX( IDispatch *obj, bool addref )
{
    unknown = nullptr;
    typeinfo = nullptr;
    memset( &variant, 0, sizeof( variant ) );
    dispatch = obj;

    if ( !dispatch )
    {
        return;
    }
    if ( addref )
    {
        dispatch->AddRef();
    }
}

ActiveX::ActiveX( IUnknown* obj, bool addref )
{
    dispatch = nullptr;
    typeinfo = nullptr;
    memset( &variant, 0, sizeof( variant ) );

    unknown = obj;
    if ( !unknown )
    {
        return;
    }

    if ( addref )
    {
        unknown->AddRef();
    }

    HRESULT hresult;

    hresult = unknown->QueryInterface( IID_IDispatch, (void * *)&dispatch );

    if ( !SUCCEEDED( hresult ) )
    {
        dispatch = 0;
    }

    // else  QueryInterface calls AddRef() for you
      //  dispatch->AddRef();
}

ActiveX::ActiveX( CLSID& clsid )
{
    HRESULT hresult;
    unknown = nullptr;
    dispatch = nullptr;
    typeinfo = nullptr;
    memset( &variant, 0, sizeof( variant ) );

    hresult = CoCreateInstance( clsid, nullptr, CLSCTX_SERVER | CLSCTX_INPROC_HANDLER,
                                IID_IUnknown, (void **)&unknown );

    if ( !SUCCEEDED( hresult ) ) 
    { 
        unknown = 0; return;
    } //throw xdb("CoCreateInstance Failure");

    hresult = unknown->QueryInterface( IID_IDispatch, (void * *)&dispatch );


    //maybe I don't know what to do with it, but it might get passed to
    //another COM function
    if ( !SUCCEEDED( hresult ) )
    {
        dispatch = nullptr;
        //unknown->Release();
        //unknown=nullptr;
        //throw xdb("IDispatch interface not found");
    }
}

ActiveX::~ActiveX()
{
    if ( dispatch )
    {
        dispatch->Release();
    }
    if ( unknown )
    {
        unknown->Release();
    }
    if ( typeinfo )
    {
        typeinfo->Release();
    }
    if ( variant.vt )
    {
        VariantClear( &variant );
    }
    CoFreeUnusedLibraries();
}

bool ActiveX::Id( std::wstring_view name, DISPID& dispid )
{
    if ( !dispatch )
    {
        return false;
    }

    dispid = 0;
    HRESULT hresult;

    if ( name.empty() || name[0] == L'0' ) 
    { 
        dispid = 0; return true; 
    }

    wchar_t* cname = const_cast<wchar_t*>(name.data());
    hresult = dispatch->GetIDsOfNames( IID_NULL, &cname, 1, LOCALE_USER_DEFAULT, &dispid );

    if ( !SUCCEEDED( hresult ) )
    {
        hresult = dispatch->GetIDsOfNames( IID_NULL, &cname, 1, LOCALE_SYSTEM_DEFAULT, &dispid );

    }
    return SUCCEEDED( hresult );
}

bool ActiveX::Invoke( DISPID dispid, JSContext* cx, unsigned argc, JS::Value* vp )
{
    if ( !dispatch )
    {
        return false;
    }

    JS::CallArgs callArgs = JS::CallArgsFromVp( argc, vp );

    VARIANT VarResult;
    VARIANTARG * args;
    DISPPARAMS dispparams = { nullptr,nullptr,0,0 };
    HRESULT hresult;
    EXCEPINFO exception = { 0 };
    UINT argerr = 0;

    if ( argc )
    {
        args = new VARIANTARG[argc];
        dispparams.rgvarg = args;
        dispparams.cArgs = argc;
        for ( size_t i = 0; i < argc; i++ )
        {
            if ( !SetupValue( args[argc - i - 1], cx, callArgs[i] ) )
            {
                args[argc - i - 1].vt = VT_ERROR;
                args[argc - i - 1].scode = 0;
            }
        }
    }

    VariantInit( &VarResult );

    // don't use DispInvoke, because we don't know the TypeInfo
    hresult = dispatch->Invoke(
        dispid,
        IID_NULL,
        LOCALE_USER_DEFAULT,
        DISPATCH_METHOD,
        &dispparams, &VarResult, &exception, &argerr );

    for ( size_t i = 0; i < argc; i++ )
    {
        CheckReturn( cx, callArgs[i] ); //in case any empty ActiveX objects were filled in by Invoke()
        VariantClear( &args[i] ); //decrement AddRefs() done in SetupValue
    }
    if ( argc ) delete[] args;

    if ( !SUCCEEDED( hresult ) )
    {
        VariantClear( &VarResult );
        callArgs.rval().setNull();
        ReportActiveXError( hresult, exception, argerr, cx );
        return false;
    }

    if ( !RetrieveValue( VarResult, cx, callArgs.rval() ) )
    {
        callArgs.rval().setNull();
    }

    VariantClear( &VarResult );
    return true;
}

bool ActiveX::Get( DISPID dispid, JSContext* cx, unsigned argc, JS::Value* vp, bool exceptions )
{
    if ( !dispatch )
    {
        return false;
    }

    JS::CallArgs callArgs = JS::CallArgsFromVp( argc, vp );

    VARIANT VarResult;
    VARIANTARG * args;
    DISPPARAMS dispparams = { nullptr,nullptr,0,0 };
    HRESULT hresult;
    EXCEPINFO exception = { 0 };
    UINT argerr = 0;

    if ( argc )
    {
        args = new VARIANTARG[argc];
        dispparams.rgvarg = args;
        dispparams.cArgs = argc;
        for ( size_t i = 0; i < argc; i++ )
        {
            if ( !SetupValue( args[argc - i - 1], cx, callArgs[i] ) )
            {
                args[argc - i - 1].vt = VT_ERROR;
                args[argc - i - 1].scode = 0;
            }
        }
    }

    VariantInit( &VarResult );

    hresult = dispatch->Invoke(
        dispid,
        IID_NULL,
        LOCALE_USER_DEFAULT,
        DISPATCH_PROPERTYGET,
        &dispparams, &VarResult, &exception, &argerr );

    for ( size_t i = 0; i < argc; i++ )
    {
        CheckReturn( cx, callArgs[i] );
        VariantClear( &args[i] );
    }

    if ( argc ) delete[] args;

    if ( !SUCCEEDED( hresult ) )
    {
        callArgs.rval().setNull();
        if ( exceptions )
        {
            ReportActiveXError( hresult, exception, argerr, cx );
        }
        return false;
    }
    else if ( !RetrieveValue( VarResult, cx, callArgs.rval() ) )
    {
        callArgs.rval().setNull();
    }
    VariantClear( &VarResult );

    return true;
}

bool ActiveX::Set( DISPID dispid, JSContext* cx, unsigned argc, JS::Value* vp, bool byref )
{
    if ( !dispatch )
    {
        return false;
    }

    JS::CallArgs callArgs = JS::CallArgsFromVp( argc, vp );

    VARIANTARG * args = new VARIANTARG[argc + 1];
    DISPID dispput = DISPID_PROPERTYPUT;
    DISPPARAMS dispparams = { args,&dispput,argc + 1,1 };
    HRESULT hresult;
    EXCEPINFO exception = { 0 };
    UINT argerr = 0;

    //the set value
    if ( !SetupValue( args[0], cx, callArgs.rval() ) )
    {
        return false;
    }

    //the index values, in reverse order
    if ( argc )
    {
        //   dispparams.rgvarg = args; //initialized in the declaration
        //   dispparams.cArgs = argc+1;
        for ( size_t i = 0; i < argc; i++ )
        {
            if ( !SetupValue( args[argc - i], cx, callArgs[i] ) )
            {
                args[argc - i].vt = VT_ERROR;
                args[argc - i].scode = 0;
            }
        }
    }

    DWORD flag = DISPATCH_PROPERTYPUT;
    if ( byref && (args[0].vt & VT_DISPATCH || args[0].vt & VT_UNKNOWN) )
    {//must be passed by name
        flag = DISPATCH_PROPERTYPUTREF;
    }

    hresult = dispatch->Invoke(
        dispid,
        IID_NULL,
        LOCALE_USER_DEFAULT,
        (WORD)flag,
        &dispparams, nullptr, &exception, &argerr ); //no result

    for ( size_t i = 0; i < argc; i++ )
    {
        CheckReturn( cx, callArgs[i] );
        VariantClear( &args[i] );
    }

    if ( argc ) delete[] args;

    if ( !SUCCEEDED( hresult ) )
    {
        ReportActiveXError( hresult, exception, argerr, cx );
        return false;
    }

    return true;
}

ActiveX::PropInfo* ActiveX::Find( std::wstring_view name )
{
    auto elem = Properties.find( name.data() );
    if ( elem == Properties.end() )
    {
        return nullptr;
    }

    return (elem->second).get();
}

bool ActiveX::SetupMembers( JSContext* cx, JS::HandleObject obj )
{
    HRESULT hresult;
    if ( unknown && !dispatch )
    {
        hresult = unknown->QueryInterface( IID_IDispatch, (void * *)&dispatch );
    }
    if ( !dispatch )
    {
        return false;
    }

    /*
    ENTERNATIVE( cx );
    */

    JS::RootedObject doc(cx, JS_NewPlainObject( cx ) );
    JS_DefineProperty( cx, obj, "members", doc, JSPROP_ENUMERATE );

    if ( !typeinfo )
    {
        unsigned ctinfo;
        hresult = dispatch->GetTypeInfoCount( &ctinfo );

        if ( SUCCEEDED( hresult ) && ctinfo )
        {
            dispatch->GetTypeInfo( 0, 0, &typeinfo );
        }
    }

    if ( !typeinfo )
    {
        return false;
    }

    size_t i;
    VARDESC * vardesc;
    for ( i = 0; typeinfo->GetVarDesc( i, &vardesc ) == S_OK && i < 255; i++ )
    {
        BSTR name = nullptr;
        BSTR desc = nullptr;
        if ( typeinfo->GetDocumentation( vardesc->memid, &name, &desc, nullptr, nullptr ) == S_OK )
        {
            PropInfo * p = Find( (wchar_t*)name );
            if ( !p )
            {
                p = new ActiveX::PropInfo( (wchar_t*)name );
            }
            p->Get = p->Put = true;
            Properties[name] = std::shared_ptr<PropInfo>( p );

            //JS_DefineUCPropertyWithTinyId
            JS_DefineUCProperty( cx, obj, (char16_t*)name, SysStringLen( name ),
                                 ActiveX_JSGet, ActiveX_JSSet, JSPROP_ENUMERATE );
            if ( doc )
            {
                JS::RootedValue d(cx);
                if ( desc && *desc )
                {
                    std::wstring wStr( desc, SysStringLen( desc ) );
                    mozjs::NativeToJsValue( cx, wStr, &d );
                }
                else
                {
                    d.setNull();
                }

                JS_DefineUCProperty( cx, doc, (char16_t*)name, SysStringLen( name ), d, JSPROP_ENUMERATE );
            }
            SysFreeString( name );
            SysFreeString( desc );
        }
        typeinfo->ReleaseVarDesc( vardesc );
    }

    FUNCDESC * funcdesc;
    for ( i = 0; typeinfo->GetFuncDesc( i, &funcdesc ) == S_OK; i++ )
    {
        BSTR name = nullptr;
        BSTR desc = nullptr;

        if ( typeinfo->GetDocumentation( funcdesc->memid, &name, &desc, nullptr, nullptr ) == S_OK )
        {
            //    char* fname = DeflateString((jschar*)name,SysStringLen(name));

            if ( funcdesc->invkind == INVOKE_FUNC )
            {
                //       JS_DefineFunction(cx,obj,fname,*ActiveX_Run,funcdesc->cParams,0);
                JS_DefineUCFunction( cx, obj, (char16_t*)name, SysStringLen( name ),
                                     *ActiveX_Run, funcdesc->cParams, 0 );
            }
            else
            {
                PropInfo * p = Find( (wchar_t*)name );

                if ( !p )
                {
                    p = new PropInfo( (wchar_t*)name );
                    Properties[name] = std::shared_ptr<PropInfo>(p);
                    JS_DefineUCProperty( cx, obj, (char16_t*)name,
                                                   SysStringLen( name ), ActiveX_JSGet, ActiveX_JSSet, JSPROP_ENUMERATE );
                }

                if ( funcdesc->invkind & INVOKE_PROPERTYGET )
                    p->Get = true;
                if ( funcdesc->invkind & INVOKE_PROPERTYPUT )
                    p->Put = true;
                if ( funcdesc->invkind & INVOKE_PROPERTYPUTREF )
                    p->PutRef = true;
            }
            //    if (fname) free(fname);

            if ( doc )
            {
                JS::RootedValue d( cx );
                if ( desc && *desc)
                {
                    std::wstring wStr( desc, SysStringLen( desc ) );
                    mozjs::NativeToJsValue( cx, wStr, &d );
                }
                else
                {
                    d.setNull();
                }

                JS_DefineUCProperty( cx, doc, (char16_t*)name, SysStringLen( name ), d, JSPROP_ENUMERATE );
            }

            SysFreeString( name );
            SysFreeString( desc );
        }
        typeinfo->ReleaseFuncDesc( funcdesc );
    }

    return true;
}

/*
CoInitialize(nullptr);
ActiveX_InitClass(cx,obj);
CoFreeUnusedLibraries();
CoUninitialize();
*/

