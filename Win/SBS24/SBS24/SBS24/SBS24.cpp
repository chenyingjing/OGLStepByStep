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

Tutorial 24 - Shadow Mapping - Part 2
*/

#include <math.h>
#include <GL/glew.h>
#include <GL/freeglut.h>

#include "ogldev_util.h"
#include "ogldev_app.h"
#include "ogldev_pipeline.h"
#include "ogldev_camera.h"
#include "lighting_technique.h"
#include "shadow_map_technique.h"
#include "ogldev_glut_backend.h"
#include "mesh.h"
#include "ogldev_shadow_map_fbo.h"

//#define WINDOW_WIDTH  1920
//#define WINDOW_HEIGHT 1200
#define WINDOW_WIDTH  1500
#define WINDOW_HEIGHT 800

class Tutorial24 : public ICallbacks, public OgldevApp
{
public:

	Tutorial24()
	{
		m_pLightingEffect = NULL;
		m_pShadowMapEffect = NULL;
		m_pGameCamera = NULL;
		m_pMesh = NULL;
		m_pQuad = NULL;
		m_scale = 0.0f;
		m_pGroundTex = NULL;

		m_spotLight.AmbientIntensity = 0.01f;
		m_spotLight.DiffuseIntensity = 0.9f;
		m_spotLight.Color = Vector3f(1.0f, 1.0f, 1.0f);
		m_spotLight.Attenuation.Linear = 0.01f;
		m_spotLight.Position = Vector3f(-20.0, 20.0, -10.0f);
		m_spotLight.Direction = Vector3f(1.0f, -1.0f, 0.0f);
		//Vector3f target(0, 0, 0);
		//m_spotLight.Direction = target - m_spotLight.Position;
		m_spotLight.Cutoff = 40.0f;

		m_persProjInfo.FOV = 60.0f;
		m_persProjInfo.Height = WINDOW_HEIGHT;
		m_persProjInfo.Width = WINDOW_WIDTH;
		m_persProjInfo.zNear = 1.0f;
		m_persProjInfo.zFar = 50.0f;

		m_scale_model = 2.0f;
		m_x_rotate = -90.0f;
		//m_x_rotate = 0.0f;
		m_y_up = 1.5f;
	}


	~Tutorial24()
	{
		SAFE_DELETE(m_pLightingEffect);
		SAFE_DELETE(m_pShadowMapEffect);
		SAFE_DELETE(m_pGameCamera);
		SAFE_DELETE(m_pMesh);
		SAFE_DELETE(m_pQuad);
		SAFE_DELETE(m_pGroundTex);
	}


	bool Init()
	{
		Vector3f Pos(0.0f, 8.0f, -20.0f);
		//Vector3f Pos(-20.0f, 20.0f, 0.0f);
		Vector3f Target(0.0f, 0.0f, 1.0f);
		//Vector3f Target(1.0f, -1.0f, 0.0f);
		Vector3f Up(0.0, 1.0f, 0.0f);

		if (!m_shadowMapFBO.Init(WINDOW_WIDTH, WINDOW_HEIGHT)) {
			return false;
		}

		m_pGameCamera = new Camera(WINDOW_WIDTH, WINDOW_HEIGHT, Pos, Target, Up);

		m_pLightingEffect = new LightingTechnique();

		if (!m_pLightingEffect->Init()) {
			printf("Error initializing the lighting technique\n");
			return false;
		}

		m_pLightingEffect->Enable();
		m_pLightingEffect->SetSpotLights(1, &m_spotLight);
		m_pLightingEffect->SetTextureUnit(0);
		m_pLightingEffect->SetShadowMapTextureUnit(1);

		m_pShadowMapEffect = new ShadowMapTechnique();

		if (!m_pShadowMapEffect->Init()) {
			printf("Error initializing the shadow map technique\n");
			return false;
		}

		m_pQuad = new Mesh();

		//if (!m_pQuad->LoadMesh("../../../Content/quad.obj")) {
		if (!m_pQuad->LoadMesh("../../../Content/quad1.obj")) {
			return false;
		}

		m_pGroundTex = new Texture(GL_TEXTURE_2D, "../../../Content/test.png");

		if (!m_pGroundTex->Load()) {
			return false;
		}

		m_pMesh = new Mesh();

		//return m_pMesh->LoadMesh("../../../Content/phoenix_ugv.md2");
		return m_pMesh->LoadMesh("../../../Content/momo/model.obj");
	}


	void Run()
	{
		GLUTBackendRun(this);
	}


	virtual void RenderSceneCB()
	{
		m_pGameCamera->OnRender();
		m_scale += 0.01f;

		ShadowMapPass();
		RenderPass();
		//RenderPass1();

		glutSwapBuffers();
	}


	void ShadowMapPass()
	{
		m_shadowMapFBO.BindForWriting();

		glClear(GL_DEPTH_BUFFER_BIT);

		m_pShadowMapEffect->Enable();

		Pipeline p;
		//p.Scale(0.1f, 0.1f, 0.1f);
		float scale_model = m_scale_model;// *1.8;
		p.Scale(scale_model, scale_model, scale_model);
		p.Rotate(m_x_rotate, m_scale, 0.0f);
		p.WorldPos(0.0f, m_y_up, 0.0f);
		p.SetCamera(m_spotLight.Position, m_spotLight.Direction, Vector3f(0.0f, 1.0f, 0.0f));
		p.SetPerspectiveProj(m_persProjInfo);
		m_pShadowMapEffect->SetWVP(p.GetWVPTrans());
		m_pMesh->Render();

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}


	void RenderPass()
	{
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		m_pLightingEffect->Enable();

		m_pLightingEffect->SetEyeWorldPos(m_pGameCamera->GetPos());

		m_shadowMapFBO.BindForReading(GL_TEXTURE1);

		Pipeline p;
		p.SetPerspectiveProj(m_persProjInfo);

		p.Scale(10.0f, 10.0f, 10.0f);
		p.WorldPos(0.0f, 0.0f, 0.0f);
		//p.Rotate(180.0f, 0.0f, 0.0f);
		p.SetCamera(m_pGameCamera->GetPos(), m_pGameCamera->GetTarget(), m_pGameCamera->GetUp());
		m_pLightingEffect->SetWVP(p.GetWVPTrans());
		m_pLightingEffect->SetWorldMatrix(p.GetWorldTrans());

		//Pipeline p1;
		////p.Scale(0.1f, 0.1f, 0.1f);
		//float scale_model = m_scale_model;// *1.8;
		//p1.Scale(scale_model, scale_model, scale_model);
		//p1.Rotate(m_x_rotate, m_scale, 0.0f);
		//p1.WorldPos(0.0f, 0.0f, 3.0f);
		p.SetCamera(m_spotLight.Position, m_spotLight.Direction, Vector3f(0.0f, 1.0f, 0.0f));
		//p1.SetPerspectiveProj(m_persProjInfo);

		m_pLightingEffect->SetLightWVP(p.GetWVPTrans());
		m_pGroundTex->Bind(GL_TEXTURE0);
		m_pQuad->Render();

		//p.Scale(0.1f, 0.1f, 0.1f);
		p.Scale(m_scale_model, m_scale_model, m_scale_model);
		p.Rotate(m_x_rotate, m_scale, 0.0f);
		p.WorldPos(0.0f, m_y_up, 0.0f);
		p.SetCamera(m_pGameCamera->GetPos(), m_pGameCamera->GetTarget(), m_pGameCamera->GetUp());
		m_pLightingEffect->SetWVP(p.GetWVPTrans());
		m_pLightingEffect->SetWorldMatrix(p.GetWorldTrans());
		p.SetCamera(m_spotLight.Position, m_spotLight.Direction, Vector3f(0.0f, 1.0f, 0.0f));
		m_pLightingEffect->SetLightWVP(p.GetWVPTrans());
		m_pMesh->Render();
	}

	void RenderPass1()
	{
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		m_pShadowMapEffect->SetTextureUnit(0);
		m_shadowMapFBO.BindForReading(GL_TEXTURE0);

		Pipeline p;
		p.Scale(10.0f, 10.0f, 10.0f);
		p.WorldPos(0.0f, 0.0f, 10.0f);
		p.SetCamera(m_pGameCamera->GetPos(), m_pGameCamera->GetTarget(), m_pGameCamera->GetUp());
		p.SetPerspectiveProj(m_persProjInfo);
		m_pShadowMapEffect->SetWVP(p.GetWVPTrans());
		m_pQuad->Render();
	}


	void KeyboardCB(OGLDEV_KEY OgldevKey, OGLDEV_KEY_STATE State)
	{
		switch (OgldevKey) {
		case OGLDEV_KEY_ESCAPE:
		case OGLDEV_KEY_q:
			GLUTBackendLeaveMainLoop();
			break;
		default:
			m_pGameCamera->OnKeyboard(OgldevKey);
		}
	}


	virtual void PassiveMouseCB(int x, int y)
	{
		m_pGameCamera->OnMouse(x, y);
	}

private:

	LightingTechnique* m_pLightingEffect;
	ShadowMapTechnique* m_pShadowMapEffect;
	Camera* m_pGameCamera;
	float m_scale;
	float m_scale_model;
	float m_x_rotate;
	float m_y_up;
	SpotLight m_spotLight;
	Mesh* m_pMesh;
	Mesh* m_pQuad;
	Texture* m_pGroundTex;
	ShadowMapFBO m_shadowMapFBO;
	PersProjInfo m_persProjInfo;
};


int main(int argc, char** argv)
{
	GLUTBackendInit(argc, argv, true, false);

	if (!GLUTBackendCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, false, "Tutorial 24")) {
		return 1;
	}

	Tutorial24* pApp = new Tutorial24();

	if (!pApp->Init()) {
		return 1;
	}

	pApp->Run();

	delete pApp;

	return 0;
}
