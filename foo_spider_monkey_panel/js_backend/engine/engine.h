#pragma once

namespace mozjs
{

class ContextInner;

}

namespace mozjs
{

class JsEngine
{
public:
    ~JsEngine();
    JsEngine( const JsEngine& ) = delete;
    JsEngine& operator=( const JsEngine& ) = delete;

    static [[nodiscard]] JsEngine& Get();
    static [[nodiscard]] ContextInner& GetContext();

    void PrepareForExit();
    void Finalize();

private:
    JsEngine();

private:
    JSContext* pJsCtx_ = nullptr;

    std::unique_ptr<ContextInner> pContext_;
    bool isInitialized_ = false;
    bool shouldShutdown_ = false;
};

} // namespace mozjs
