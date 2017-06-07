/*
Copyright 2011 Etay Meiri

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/


#include <stdio.h>

#include "shadow_map_fbo.h"


ShadowMapFBO::ShadowMapFBO()
{
	m_fbo = 0;
	m_shadowMap = 0;
    _offscreenColorRenderBuffer = 0;
    _depthRenderBuffer = 0;
}

ShadowMapFBO::~ShadowMapFBO()
{
	if (m_fbo != 0) {
		glDeleteFramebuffers(1, &m_fbo);
	}

	if (m_shadowMap != 0) {
		glDeleteTextures(1, &m_shadowMap);
	}
}

bool ShadowMapFBO::Init(unsigned int WindowWidth, unsigned int WindowHeight)
{
    glGenFramebuffers(1, &m_fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
    
//    glGenRenderbuffers(1, &_offscreenColorRenderBuffer);
//    glBindRenderbuffer(GL_RENDERBUFFER, _offscreenColorRenderBuffer);
//    glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA8_OES, WindowWidth, WindowHeight);
//    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
//                              GL_RENDERBUFFER, _offscreenColorRenderBuffer);
//    
//    
//    
//    glGenRenderbuffers(1, &_depthRenderBuffer);
//    glBindRenderbuffer(GL_RENDERBUFFER, _depthRenderBuffer);
//    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, WindowWidth, WindowHeight);
//    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
//                              GL_RENDERBUFFER, _depthRenderBuffer);
    
    
    glGenTextures(1, &m_shadowMap);
    glBindTexture(GL_TEXTURE_2D, m_shadowMap);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, WindowWidth, WindowHeight, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, NULL);
    checkError();
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_shadowMap, 0);
    
    // Check FBO satus
    //
    GLenum Status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (Status != GL_FRAMEBUFFER_COMPLETE) {
        printf("FB error, status: 0x%x\n", Status);
        return false;
    }
    return true;

}


void ShadowMapFBO::BindForWriting()
{
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER_APPLE, m_fbo);
}


void ShadowMapFBO::BindForReading(GLenum TextureUnit)
{
	glActiveTexture(TextureUnit);
	glBindTexture(GL_TEXTURE_2D, m_shadowMap);
}
