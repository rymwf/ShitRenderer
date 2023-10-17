/**
 * @file GLFramebuffer.hpp
 * @author yangzs
 * @brief
 * @version 0.1
 *
 * @copyright Copyright (c) 2021
 *
 */
#pragma once
#include <renderer/ShitFramebuffer.hpp>

#include "GLPrerequisites.hpp"
namespace Shit {
/**
 * @brief canbe fbo or default framebuffer
 * @brief ca
 *
 */
class GLFramebuffer final : public Framebuffer {
public:
    struct ColorAttachment {
        GLenum attachPoint;
        GLuint writeMask{15};
        GLenum objectType;                // GL_NONE, GL_FRAMEBUFFER_DEFAULT, GL_TEXTURE, or
                                          // GL_RENDERBUFFER
        GLenum colorEncoding{GL_LINEAR};  // GL_LINEAR or GL_SRGB
    };
    struct DepthStencilAttachment {
        GLenum attachPoint;
        bool depthWriteMask{true};
        GLuint stencilWriteMask{1};
        GLuint stencilBackWriteMask{1};
        GLenum objectType;  // GL_NONE, GL_FRAMEBUFFER_DEFAULT, GL_TEXTURE, or
                            // GL_RENDERBUFFER
    };

    struct FBOState {
        GLuint handle{};  // when handle is 0, color and depthstencil attachments
                          // are none

        std::vector<ColorAttachment> colorAttachments;
        DepthStencilAttachment depthStencilAttachment;

        GLenum readBuffer;
        std::vector<GLenum> drawBuffers;

        std::stack<std::vector<GLenum>> drawBuffersStack;
        std::stack<GLenum> readBufferStack;

        void BindReadBuffer(GLenum mode);
        void BindDrawBuffers(std::span<const GLenum> modes);
        void BindDrawBuffer(GLenum mode);

        void PushReadBuffer(GLenum mode);
        void PopReadBuffer();

        void PushDrawBuffers(std::span<const GLenum> modes);
        void PopDrawBuffers();

        void PushDrawBuffer(GLenum mode);
        void PopDrawBuffer();
    };

    GLFramebuffer(GLStateManager *pStateManager, const FramebufferCreateInfo &createInfo);
    ~GLFramebuffer();

    void BindFBO(uint32_t fboIndex, bool isRenderFBO, bool isDraw) const;

    void Resolve(uint32_t fboIndex, Filter filter) const;

    void SetColorMask(uint32_t fboIndex, uint32_t attachmentIndex, GLuint writeMask);
    void SetDepthMask(uint32_t fboIndex, bool writeMask);
    void SetStencilMask(uint32_t fboIndex, GLuint writeMask);
    void SetStencilBackMask(uint32_t fboIndex, GLuint writeMask);

private:
    GLStateManager *mpStateManager;

    bool mMultiview = false;

    // create fbo for every subpass, first is render fbo, second is resolveFBO
    // the order is render subpass order
    mutable std::vector<std::pair<FBOState, std::unique_ptr<FBOState>>> mFBOs;

    static void CheckCompleteness();

    void BindFBOAttachment(GLenum bindpoint, const GLImageView *imageView);

    void CreateFBO(uint32_t fboIndex, bool isRenderFBO);

    void ClearBuffer(uint32_t fboIndex) const;
};
}  // namespace Shit
