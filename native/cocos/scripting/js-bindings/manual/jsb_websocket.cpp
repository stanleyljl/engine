#include "jsb_websocket.hpp"

#include "cocos/scripting/js-bindings/jswrapper/SeApi.h"
#include "cocos/scripting/js-bindings/manual/jsb_conversions.hpp"
#include "cocos/scripting/js-bindings/manual/jsb_global.h"

#include "cocos/network/WebSocket.h"
#include "base/ccUTF8.h"
#include "base/CCDirector.h"

using namespace cocos2d::network;

/*
 [Constructor(in DOMString url, in optional DOMString protocols)]
 [Constructor(in DOMString url, in optional DOMString[] protocols)]
 interface WebSocket {
 readonly attribute DOMString url;

 // ready state
 const unsigned short CONNECTING = 0;
 const unsigned short OPEN = 1;
 const unsigned short CLOSING = 2;
 const unsigned short CLOSED = 3;
 readonly attribute unsigned short readyState;
 readonly attribute unsigned long bufferedAmount;

 // networking
 attribute Function onopen;
 attribute Function onmessage;
 attribute Function onerror;
 attribute Function onclose;
 readonly attribute DOMString protocol;
 void send(in DOMString data);
 void close();
 };
 WebSocket implements EventTarget;
 */

se::Class* __jsb_WebSocket_class = nullptr;

class JSB_WebSocketDelegate : public WebSocket::Delegate
{
public:

    JSB_WebSocketDelegate()
    {
    }

    ~JSB_WebSocketDelegate()
    {
        CCLOGINFO("In the destructor of JSB_WebSocketDelegate(%p)", this);
    }

    virtual void onOpen(WebSocket* ws) override
    {
        se::ScriptEngine::getInstance()->clearException();
        se::AutoHandleScope hs;

        if (cocos2d::Director::getInstance() == nullptr || cocos2d::ScriptEngineManager::getInstance() == nullptr)
            return;

        auto iter = se::__nativePtrToObjectMap.find(ws);
        if (iter == se::__nativePtrToObjectMap.end())
            return;

        se::Object* wsObj = iter->second;
        wsObj->setProperty("protocol", se::Value(ws->getProtocol()));
        se::Object* jsObj = se::Object::createPlainObject(true);
        jsObj->setProperty("type", se::Value("open"));

        se::Value func;
        bool ok = _JSDelegate.toObject()->getProperty("onopen", &func);
        if (ok)
        {
            se::ValueArray args;
            args.push_back(se::Value(jsObj));
            func.toObject()->call(args, wsObj);
        }
        else
        {
            SE_REPORT_ERROR("Can't get onopen function!");
        }

        jsObj->switchToUnrooted();
        jsObj->release();
    }

    virtual void onMessage(WebSocket* ws, const WebSocket::Data& data) override
    {
        se::ScriptEngine::getInstance()->clearException();
        se::AutoHandleScope hs;

        if (cocos2d::Director::getInstance() == nullptr || cocos2d::ScriptEngineManager::getInstance() == nullptr)
            return;

        auto iter = se::__nativePtrToObjectMap.find(ws);
        if (iter == se::__nativePtrToObjectMap.end())
            return;

        se::Object* wsObj = iter->second;
        se::Object* jsObj = se::Object::createPlainObject(true);
        jsObj->setProperty("type", se::Value("message"));

        se::Value func;
        bool ok = _JSDelegate.toObject()->getProperty("onmessage", &func);
        if (ok)
        {
            se::ValueArray args;
            args.push_back(se::Value(jsObj));

            if (data.isBinary)
            {
                se::Object* dataObj = se::Object::createArrayBufferObject(data.bytes, data.len, false);
                jsObj->setProperty("data", se::Value(dataObj));
                dataObj->release();
            }
            else
            {
                se::Value dataVal;
                if (strlen(data.bytes) == 0 && data.len > 0)
                {// String with 0x00 prefix
                    std::string str(data.bytes, data.len);
                    dataVal.setString(str);
                }
                else
                {// Normal string
                    dataVal.setString(data.bytes);
                }
                if (dataVal.isNullOrUndefined())
                {
                    ws->closeAsync();
                }
                else
                {
                    jsObj->setProperty("data", se::Value());
                    func.toObject()->call(args, wsObj);
                }
            }

            func.toObject()->call(args, wsObj);
        }
        else
        {
            SE_REPORT_ERROR("Can't get onmessage function!");
        }

        jsObj->switchToUnrooted();
        jsObj->release();
    }

    virtual void onClose(WebSocket* ws) override
    {
        se::ScriptEngine::getInstance()->clearException();
        se::AutoHandleScope hs;

        if (cocos2d::Director::getInstance() == nullptr || cocos2d::ScriptEngineManager::getInstance() == nullptr)
            return;

        auto iter = se::__nativePtrToObjectMap.find(ws);
        do
        {
            if (iter == se::__nativePtrToObjectMap.end())
            {
                CCLOGINFO("WebSocket js instance was destroyted, don't need to invoke onclose callback!");
                break;
            }

            se::Object* wsObj = iter->second;
            se::Object* jsObj = se::Object::createPlainObject(true);
            jsObj->setProperty("type", se::Value("close"));

            se::Value func;
            bool ok = _JSDelegate.toObject()->getProperty("onclose", &func);
            if (ok)
            {
                se::ValueArray args;
                args.push_back(se::Value(jsObj));
                func.toObject()->call(args, wsObj);
            }
            else
            {
                SE_REPORT_ERROR("Can't get onclose function!");
            }

            jsObj->switchToUnrooted();
            jsObj->release();

        } while(false);
    }

    virtual void onError(WebSocket* ws, const WebSocket::ErrorCode& error) override
    {
        se::ScriptEngine::getInstance()->clearException();
        se::AutoHandleScope hs;

        if (cocos2d::Director::getInstance() == nullptr || cocos2d::ScriptEngineManager::getInstance() == nullptr)
            return;

        auto iter = se::__nativePtrToObjectMap.find(ws);
        if (iter == se::__nativePtrToObjectMap.end())
            return;

        se::Object* wsObj = iter->second;
        se::Object* jsObj = se::Object::createPlainObject(true);
        jsObj->setProperty("type", se::Value("error"));

        se::Value func;
        bool ok = _JSDelegate.toObject()->getProperty("onerror", &func);
        if (ok)
        {
            se::ValueArray args;
            args.push_back(se::Value(jsObj));
            func.toObject()->call(args, wsObj);
        }
        else
        {
            SE_REPORT_ERROR("Can't get onerror function!");
        }

        jsObj->switchToUnrooted();
        jsObj->release();
    }

    void setJSDelegate(const se::Value& jsDelegate)
    {
        assert(jsDelegate.isObject());
        _JSDelegate = jsDelegate;
    }
private:
    se::Value _JSDelegate;
};

static bool WebSocket_finalize(se::State& s)
{
    WebSocket* cobj = (WebSocket*)s.nativeThisObject();
    CCLOGINFO("jsbindings: finalizing JS object %p (WebSocket)", cobj);

    WebSocket::Delegate* delegate = cobj->getDelegate();

    auto hook = std::make_shared<WebSocket::AfterCloseHook>([=](){
        // Delete WebSocket delegate
        delete delegate;
        // Delete WebSocket instance
        delete cobj;
    });
    cobj->setAfterCloseHook(hook);

    // Manually close if web socket is not closed
    if (cobj->getReadyState() != WebSocket::State::CLOSED)
    {
        CCLOGINFO("WebSocket (%p) isn't closed, try to close it!", cobj);
        cobj->closeAsync();
    }
    return true;
}
SE_BIND_FINALIZE_FUNC(WebSocket_finalize)

static bool WebSocket_constructor(se::State& s)
{
    const auto& args = s.args();
    int argc = (int)args.size();

    if (argc == 1 || argc == 2 || argc == 3)
    {
        std::string url;

        bool ok = seval_to_std_string(args[0], &url);
        JSB_PRECONDITION2(ok, false, "Error processing url argument");

        se::Object* obj = s.thisObject();
        WebSocket* cobj = nullptr;
        if (argc >= 2)
        {
            std::string caFilePath;
            std::vector<std::string> protocols;

            if (args[1].isString())
            {
                std::string protocol;
                ok = seval_to_std_string(args[1], &protocol);
                JSB_PRECONDITION2(ok, false, "Error processing protocol string");
                protocols.push_back(protocol);
            }
            else if (args[1].isObject() && args[1].toObject()->isArray())
            {
                se::Object* protocolArr = args[1].toObject();
                uint32_t len = 0;
                ok = protocolArr->getArrayLength(&len);
                JSB_PRECONDITION2(ok, false, "getArrayLength failed!");

                se::Value tmp;
                for (uint32_t i=0; i < len; ++i)
                {
                    if (!protocolArr->getArrayElement(i, &tmp))
                        continue;

                    std::string protocol;
                    ok = seval_to_std_string(tmp, &protocol);
                    JSB_PRECONDITION2(ok, false, "Error processing protocol object");
                    protocols.push_back(protocol);
                }
            }

            if (argc > 2)
            {
                ok = seval_to_std_string(args[2], &caFilePath);
                JSB_PRECONDITION2(ok, false, "Error processing caFilePath");
            }

            cobj = new (std::nothrow) WebSocket();
            JSB_WebSocketDelegate* delegate = new (std::nothrow) JSB_WebSocketDelegate();
            delegate->setJSDelegate(se::Value(obj));
            cobj->init(*delegate, url, &protocols, caFilePath);
        }
        else
        {
            cobj = new (std::nothrow) WebSocket();
            JSB_WebSocketDelegate* delegate = new (std::nothrow) JSB_WebSocketDelegate();
            delegate->setJSDelegate(se::Value(obj));
            cobj->init(*delegate, url);
        }

        obj->setProperty("url", args[0]);

        // The websocket draft uses lowercase 'url', so 'URL' need to be deprecated.
        obj->setProperty("URL", args[0]);

        // Initialize protocol property with an empty string, it will be assigned in onOpen delegate.
        obj->setProperty("protocol", se::Value(""));

        obj->setPrivateData(cobj);
        obj->addRef();

        return true;
    }

    SE_REPORT_ERROR("wrong number of arguments: %d, was expecting 1<= and <=3", argc);
    return false;
}
SE_BIND_CTOR(WebSocket_constructor, __jsb_WebSocket_class, WebSocket_finalize)

static bool WebSocket_send(se::State& s)
{
    const auto& args = s.args();
    int argc = (int)args.size();

    if (argc == 1)
    {
        WebSocket* cobj = (WebSocket*)s.nativeThisObject();
        bool ok = false;
        if (args[0].isString())
        {
            std::string data;
            ok = seval_to_std_string(args[0], &data);
            JSB_PRECONDITION2(ok, false, "Convert string failed");
//FIXME: We didn't find a way to get the JS string length in JSB2.0.
//            if (data.empty() && len > 0)
//            {
//                CCLOGWARN("Text message to send is empty, but its length is greater than 0!");
//                //FIXME: Note that this text message contains '0x00' prefix, so its length calcuted by strlen is 0.
//                // we need to fix that if there is '0x00' in text message,
//                // since javascript language could support '0x00' inserted at the beginning or the middle of text message
//            }

            cobj->send(data);
        }
        else if (args[0].isObject())
        {
            se::Object* dataObj = args[0].toObject();
            uint8* ptr = nullptr;
            size_t length = 0;
            if (dataObj->isArrayBuffer())
            {
                ok = dataObj->getArrayBufferData(&ptr, &length);
                JSB_PRECONDITION2(ok, false, "getArrayBufferData failed!");
            }
            else if (dataObj->isTypedArray())
            {
                ok = dataObj->getTypedArrayData(&ptr, &length);
                JSB_PRECONDITION2(ok, false, "getTypedArrayData failed!");
            }
            else
            {
                assert(false);
            }

            cobj->send(ptr, (unsigned int)length);
        }
        else
        {
            assert(false);
        }

        return true;
    }
    SE_REPORT_ERROR("wrong number of arguments: %d, was expecting 1", argc);
    return false;
}
SE_BIND_FUNC(WebSocket_send)

static bool WebSocket_close(se::State& s)
{
    const auto& args = s.args();
    int argc = (int)args.size();

    if (argc == 0)
    {
        WebSocket* cobj = (WebSocket*)s.nativeThisObject();
        cobj->closeAsync();
        return true;
    }
    SE_REPORT_ERROR("wrong number of arguments: %d, was expecting 0", argc);
    return false;
}
SE_BIND_FUNC(WebSocket_close)

static bool WebSocket_getReadyState(se::State& s)
{
    const auto& args = s.args();
    int argc = (int)args.size();

    if (argc == 0)
    {
        WebSocket* cobj = (WebSocket*)s.nativeThisObject();
        s.rval().setInt32((int)cobj->getReadyState());
        return true;
    }
    SE_REPORT_ERROR("wrong number of arguments: %d, was expecting 0", argc);
    return false;
}
SE_BIND_PROP_GET(WebSocket_getReadyState)

bool register_all_websocket(se::Object* obj)
{
    se::Class* cls = se::Class::create("WebSocket", obj, nullptr, _SE(WebSocket_constructor));
    cls->defineFinalizedFunction(_SE(WebSocket_finalize));

    cls->defineFunction("send", _SE(WebSocket_send));
    cls->defineFunction("close", _SE(WebSocket_close));
    cls->defineProperty("readyState", _SE(WebSocket_getReadyState), nullptr);

    cls->install();

    se::Value tmp;
    obj->getProperty("WebSocket", &tmp);
    tmp.toObject()->setProperty("CONNECTING", se::Value((int)WebSocket::State::CONNECTING));
    tmp.toObject()->setProperty("OPEN", se::Value((int)WebSocket::State::OPEN));
    tmp.toObject()->setProperty("CLOSING", se::Value((int)WebSocket::State::CLOSING));
    tmp.toObject()->setProperty("CLOSED", se::Value((int)WebSocket::State::CLOSED));

    JSBClassType::registerClass<WebSocket>(cls);

    __jsb_WebSocket_class = cls;

    return true;
}
