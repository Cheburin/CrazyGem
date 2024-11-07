#include "main.h"

#include "DXUTgui.h"
#include "SDKmisc.h"

extern GraphicResources * G;

extern SwapChainGraphicResources * SCG;

extern SceneState scene_state;

extern BlurHandling blur_handling;

extern CDXUTTextHelper*                    g_pTxtHelper;

ID3D11ShaderResourceView* null[] = { nullptr, nullptr, nullptr, nullptr, nullptr, nullptr };

inline void set_scene_constant_buffer(ID3D11DeviceContext* context){
	G->scene_constant_buffer->SetData(context, scene_state);
};

inline void set_blur_constant_buffer(ID3D11DeviceContext* context){
	//G->blur_constant_buffer->SetData(context, blur_handling);
};

void RenderText()
{
	g_pTxtHelper->Begin();
	g_pTxtHelper->SetInsertionPos(2, 0);
	g_pTxtHelper->SetForegroundColor(D3DXCOLOR(1.0f, 1.0f, 0.0f, 1.0f));
	g_pTxtHelper->DrawTextLine(DXUTGetFrameStats(true && DXUTIsVsyncEnabled()));
	g_pTxtHelper->DrawTextLine(DXUTGetDeviceStats());

	g_pTxtHelper->End();
}
DirectX::XMFLOAT4 lightDirection;
DirectX::XMFLOAT3 lightPosition;
DirectX::XMFLOAT3 lightLookAt;

/////
SimpleMath::Vector2 getDistanceBetweenSkewLines(SimpleMath::Vector3 S1, SimpleMath::Vector3 V1, SimpleMath::Vector3 S2, SimpleMath::Vector3 V2){
	using namespace SimpleMath;
	float c = 1.0f / (pow(V1.Dot(V2), 2.0f) - V1.Dot(V1)*V2.Dot(V2));
	float a = (S2 - S1).Dot(V1);
	float b = (S2 - S1).Dot(V2);
	Vector2 ret;
	ret.x = c*(-V2.Dot(V2)*a + V1.Dot(V2)*b);
	ret.y = c*(-V1.Dot(V2)*a + V1.Dot(V1)*b);
	return ret;
}
bool getLowestRoot(float a, float b, float c, float maxR, float* root)
{
	// Check if a solution exists
	float determinant = b*b - 4.0f*a*c;
	// If determinant is negative it means no solutions.
	if (determinant < 0.0f) return false;
	// calculate the two roots: (if determinant == 0 then
	// x1==x2 but lets disregard that slight optimization)
	float sqrtD = sqrt(determinant);
	float r1 = (-b - sqrtD) / (2 * a);
	float r2 = (-b + sqrtD) / (2 * a);
	// Sort so x1 <= x2
	if (r1 > r2) {
		float temp = r2;
		r2 = r1;
		r1 = temp;
	}
	//*root = r1;
	//return true;
	// Get lowest root:
	if (0.0f <= r1 && r1 <= maxR) {
		*root = r1;
		return true;
	}
	// It is possible that we want x2 - this can happen
	// if x1 < 0
	if (0.0f <= r2 && r2 <= maxR) {
		*root = r2;
		return true;
	}

	// No (valid) solutions
	return false;
}
SimpleMath::Vector4 vec4(SimpleMath::Vector3 v3, float w)
{
	return SimpleMath::Vector4(v3.x, v3.y, v3.z, w);
}
namespace {
	SimpleMath::Vector3 normalize(SimpleMath::Vector3 v)
	{
		v.Normalize();
		return v;
	}
}
SimpleMath::Vector3 projectOnVector(const SimpleMath::Vector3 & vector, const SimpleMath::Vector3 & onVector)
{
	return (onVector.Dot(vector) / onVector.Dot(onVector)) * onVector;
}
SimpleMath::Vector2 projectOnZ(const SimpleMath::Vector3& v)
{
	return SimpleMath::Vector2(v.x, v.z);
}
struct Cylinder{
	float r;
	float cylinderRcylinderR;
	SimpleMath::Vector3 A;
	SimpleMath::Vector3 B;
	SimpleMath::Vector3 BA;
	SimpleMath::Vector3 vX[2];
	SimpleMath::Matrix ToCylinderLocalSpace;
	Cylinder(float _1, float _2, float _3, float _4, float _5, float _6)
	{
		r = 5;
		cylinderRcylinderR = r*r;

		A.x = _1;
		A.y = _2;
		A.z = _3;
		B.x = _4;
		B.y = _5;
		B.z = _6;

		BA = B - A;
		vX[0] = A;
		vX[1] = B;

		{
			// std make frame
			// trivial impl
			SimpleMath::Vector3 X(1, 0, 0);
			auto Y = BA;
			Y.Normalize();

			// check X collinearity
			if (1.0f - abs(X.Dot(Y)) < 0.001f)
			{
				X = SimpleMath::Vector3(0, 1, 0);
			}

			auto Z = X.Cross(Y);
			X = Y.Cross(Z);

			X.Normalize();
			Y.Normalize();
			Z.Normalize();
			// Cylinder it is Y
			ToCylinderLocalSpace = SimpleMath::Matrix(X, Y, Z);
			ToCylinderLocalSpace = ToCylinderLocalSpace.Transpose();
		}
	}
};
struct Line{
	SimpleMath::Vector3 A;
	SimpleMath::Vector3 B;
	SimpleMath::Vector3 BA;
	SimpleMath::Vector3 vX[2];
	Line(float _1, float _2, float _3, float _4, float _5, float _6)
	{
		A.x = _1;
		A.y = _2;
		A.z = _3;
		B.x = _4;
		B.y = _5;
		B.z = _6;

		BA = B - A;
		vX[0] = A;
		vX[1] = B;
	}
};
float correctMistake(float t, float r, const SimpleMath::Vector4 & plane, const SimpleMath::Vector3 & prev_origin, const SimpleMath::Vector3 & origin)
{
	auto dist_prev_origin = plane.Dot(vec4(prev_origin, 1.0f));
	auto dist_origin = plane.Dot(vec4(origin, 1.0f));
	if (dist_origin < r)
	{
		t = t - (r - dist_origin) / (dist_prev_origin - dist_origin) * t;
	}
	return t < 0.0f ? 0.0f : t;
}
float offsetBySmallDistantce(float t, float worldDistanceLength)
{
	t = t - 0.05f / worldDistanceLength;
	return t < 0.0f ? 0.0f : t;
}
Cylinder cylinder(0, 0, 0, 0, 20, 0);// 50, 2, 8, 11, 0, 50);// 30, 20 + 20, 10, 0, 40 + 20, -10);
Line line(50, 2, 8, 11, 0, 50);

/////

SimpleMath::Vector3 velocity(0, 0, 0);// -5 * 0.2, -7 * 0.2, -8 * 0.2);
extern bool fire;
extern bool forcePush;
void CALLBACK OnFrameMove(double fTime, float fElapsedTime, void* pUserContext)
{
	static float angle = 270.0f;
	float radians;
	static float offsetX = 9.0f;


	// Update direction of the light.
	angle -= 0.03f * fElapsedTime * 1000;
	if (angle < 90.0f)
	{
		angle = 270.0f;
		offsetX = 9.0f;
	}
	radians = angle * 0.0174532925f;
	lightDirection = DirectX::XMFLOAT4(-5-40, 0-80, 0.0f, 0.0f);

	// Update the lookat and position.
	offsetX -= 0.003f * fElapsedTime * 1000;
	lightPosition = DirectX::XMFLOAT3(0.0f + offsetX, 10.0f, 1.0f);
	lightLookAt = DirectX::XMFLOAT3(0.0f - offsetX, 0.0f, 2.0f);

	/////////////////////////////////////////////////////////////////////////////////
	lightPosition = DirectX::XMFLOAT3(40.0f, 80.0f, 0.0f);
	lightLookAt = DirectX::XMFLOAT3(-5, 0.0f, 0.0f);
	auto ld = SimpleMath::Vector3(lightLookAt) - SimpleMath::Vector3(lightPosition);
	ld.Normalize();
	lightDirection = SimpleMath::Vector4(ld);
	/////////////////////////////////////////////////////////////////////////////////

	////for test collision
	{
		if (!fire && velocity.Length() == .0){
			SimpleMath::Matrix m = scene_state.mInvView;
			m = m.Transpose();
			m = SimpleMath::Matrix::CreateTranslation(0, 0, 25) * m;
			cylinder.A = SimpleMath::Vector3::Transform(SimpleMath::Vector3(0, 0, 0), m);
			cylinder.B = SimpleMath::Vector3::Transform(SimpleMath::Vector3(0, 20, 0), m);
		}
	}
	{
		if (fire && velocity.Length() == .0 ){
			SimpleMath::Matrix m = scene_state.mInvView;
			m = m.Transpose();
			SimpleMath::Vector3 f = -1.0 * m.Forward();
			f.Normalize();
			velocity = 2.0f*f;
		}
	}
	////

	Camera::OnFrameMove(fTime, fElapsedTime, pUserContext);
}

void renderSceneIntoGBuffer(ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dImmediateContext);
void postProccessGBuffer(ID3D11Device* pd3dDevice, ID3D11DeviceContext* context);
void postProccessBlur(ID3D11Device* pd3dDevice, ID3D11DeviceContext* context, _In_opt_ std::function<void __cdecl()> setHState, _In_opt_ std::function<void __cdecl()> setVState);

void ortho_box_set_world_matrix2(SimpleMath::Matrix w);

void clearAndSetRenderTarget(ID3D11DeviceContext* context, float ClearColor[], int n, ID3D11RenderTargetView** pRTV, ID3D11DepthStencilView* pDSV){
	for (int i = 0; i < n; i++)
		context->ClearRenderTargetView(pRTV[i], ClearColor);

	context->ClearDepthStencilView(pDSV, D3D11_CLEAR_DEPTH, 1.0f, 0);

	context->OMSetRenderTargets(n, pRTV, pDSV); //renderTargetViewToArray(pRTV) DXUTGetD3D11RenderTargetView
}
void renderScene(ID3D11Device* pd3dDevice, ID3D11DeviceContext* context, float fElapsedTime, IEffect* effect, bool renderIntoShadowMap);
void CALLBACK OnD3D11FrameRender(ID3D11Device* pd3dDevice, ID3D11DeviceContext* context,
	double fTime, float fElapsedTime, void* pUserContext)
{
	float clearColor[4] = { 0.0f, 0.5f, 0.8f, 1.0f };
	D3D11_VIEWPORT vp;
	vp.TopLeftX = 0;
	vp.TopLeftY = 0;
	vp.Width = 1024;
	vp.Height = 1024;
	vp.MinDepth = 0;
	vp.MaxDepth = 1;
	context->RSSetViewports(1, &vp);

	{
		context->PSSetShaderResources(0, 5, null);
	}
	{
		clearAndSetRenderTarget(context, clearColor, 1, renderTargetViewToArray(SCG->depthV.Get()), SCG->depthStencil1V.Get());// SCG->depthStencil1V.Get());// SCG->depthStencilV.Get()); ///SCG->colorV.Get(), SCG->normalV.Get()

		renderScene(pd3dDevice, context, fElapsedTime, G->model_effect1.get(), true);
	}
	{
		D3D11_VIEWPORT vp;
		vp.TopLeftX = 0;
		vp.TopLeftY = 0;
		vp.Width = scene_state.vFrustumParams.x;
		vp.Height = scene_state.vFrustumParams.y;
		vp.MinDepth = 0;
		vp.MaxDepth = 1;
		context->RSSetViewports(1, &vp);

		clearAndSetRenderTarget(context, clearColor, 1, renderTargetViewToArray(DXUTGetD3D11RenderTargetView()), DXUTGetD3D11DepthStencilView());// SCG->depthStencil1V.Get());// SCG->depthStencilV.Get()); ///SCG->colorV.Get(), SCG->normalV.Get()

		context->PSSetShaderResources(2, 1, shaderResourceViewToArray(SCG->depthSRV.Get()));

		renderScene(pd3dDevice, context, fElapsedTime, G->model_effect2.get(), false);

		{
			context->GSSetConstantBuffers(0, 1, constantBuffersToArray(*(G->scene_constant_buffer)));

			ortho_box_set_world_matrix();

			set_scene_constant_buffer(context);

			ortho_box_draw(context, G->ortho_box_effect.get(), 0, [=]{
				context->OMSetBlendState(G->render_states->Opaque(), Colors::Black, 0xFFFFFFFF);
				context->RSSetState(G->render_states->Wireframe());
				context->OMSetDepthStencilState(G->render_states->DepthDefault(), 0);
			});
		}
	}
	{
		context->PSSetShaderResources(0, 5, null);
	}

}
/*
void CALLBACK OnD3D11FrameRender(ID3D11Device* pd3dDevice, ID3D11DeviceContext* context,
	double fTime, float fElapsedTime, void* pUserContext)
{
	float clearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };

	clearAndSetRenderTarget(context, clearColor, 1, renderTargetViewToArray(DXUTGetD3D11RenderTargetView()), DXUTGetD3D11DepthStencilView());

	context->VSSetConstantBuffers(0, 1, constantBuffersToArray(*(G->scene_constant_buffer)));

	context->PSSetSamplers(0, 2, samplerStateToArray(G->render_states->AnisotropicClamp(), G->render_states->AnisotropicWrap()));

	context->PSSetShaderResources(2, 1, shaderResourceViewToArray(G->projected_texture.Get()));

	plane_set_world_matrix();

	set_scene_constant_buffer(context);

	plane_draw(context, G->model_with_texture.get(), G->model_input_layout.Get(), [=]{
		context->PSSetShaderResources(0, 1, shaderResourceViewToArray(G->metal_texture.Get()));

		context->OMSetBlendState(G->render_states->Opaque(), Colors::Black, 0xFFFFFFFF);
		context->RSSetState(G->render_states->CullCounterClockwise());
		context->OMSetDepthStencilState(G->render_states->DepthDefault(), 0);
	});

	cube_set_world_matrix();

	set_scene_constant_buffer(context);

	cube_draw(context, G->model_with_texture.get(), G->model_input_layout.Get(), [=]{
		context->PSSetShaderResources(0, 1, shaderResourceViewToArray(G->wall_texture.Get()));

		context->OMSetBlendState(G->render_states->Opaque(), Colors::Black, 0xFFFFFFFF);
		context->RSSetState(G->render_states->CullCounterClockwise());
		context->OMSetDepthStencilState(G->render_states->DepthDefault(), 0);
	});
}
*/
/*
void CALLBACK OnD3D11FrameRender(ID3D11Device* pd3dDevice, ID3D11DeviceContext* context,
	double fTime, float fElapsedTime, void* pUserContext)
{
	D3D11_VIEWPORT vp;
	vp.TopLeftX = 0;
	vp.TopLeftY = 0;
	vp.Width = scene_state.vFrustumParams.x;
	vp.Height = scene_state.vFrustumParams.y;
	vp.MinDepth = 0;
	vp.MaxDepth = 1;
	context->RSSetViewports(1, &vp);

	float clearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };

	{
		context->PSSetShaderResources(0, 5, null);
	}
	{
		clearAndSetRenderTarget(context, clearColor, 1, renderTargetViewToArray(SCG->depthV.Get()), DXUTGetD3D11DepthStencilView());// SCG->depthStencil1V.Get());// SCG->depthStencilV.Get()); ///SCG->colorV.Get(), SCG->normalV.Get()

		renderScene(pd3dDevice, context, G->model_effect1.get());

		clearAndSetRenderTarget(context, clearColor, 1, renderTargetViewToArray(SCG->colorV.Get()), DXUTGetD3D11DepthStencilView());// SCG->depthStencil1V.Get());// SCG->depthStencilV.Get()); ///SCG->colorV.Get(), SCG->normalV.Get()

			context->PSSetShaderResources(2, 1, shaderResourceViewToArray(SCG->depthSRV.Get()));

		renderScene(pd3dDevice, context, G->model_effect2.get());

		//clearAndSetRenderTarget(context, clearColor, 1, renderTargetViewToArray(SCG->colorV.Get()), SCG->depthStencil2V.Get());// SCG->depthStencilV.Get()); ///SCG->colorV.Get(), SCG->normalV.Get()

		//renderScene(pd3dDevice, context, G->model_effect2.get());
	}
	{
		context->PSSetShaderResources(0, 5, null);
	}
	postProccessBlur(pd3dDevice, context,
		[=]{
			float clearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
			clearAndSetRenderTarget(context, clearColor, 1, renderTargetViewToArray(SCG->colorSecondV.Get()), DXUTGetD3D11DepthStencilView());
			context->PSSetShaderResources(0, 1, shaderResourceViewToArray(SCG->colorSRV.Get()));
		},
		[=]{
			float clearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
			context->PSSetShaderResources(0, 5, null);
			clearAndSetRenderTarget(context, clearColor, 1, renderTargetViewToArray(SCG->colorV.Get()), DXUTGetD3D11DepthStencilView());
			context->PSSetShaderResources(0, 1, shaderResourceViewToArray(SCG->colorSecondSRV.Get()));
		}
	);
	{
		context->PSSetShaderResources(0, 5, null);
	}
	ID3D11RenderTargetView* rt;
	{
		clearAndSetRenderTarget(context, clearColor, 1, renderTargetViewToArray(DXUTGetD3D11RenderTargetView()), DXUTGetD3D11DepthStencilView());

			//context->PSSetShaderResources(1, 2, shaderResourceViewToArray(SCG->depthStencil1SRV.Get(), SCG->depthStencil2SRV.Get()));
			context->PSSetShaderResources(1, 1, shaderResourceViewToArray(SCG->colorSRV.Get()));

		renderScene(pd3dDevice, context, G->model_shadow_effect.get());

		//postProccessGBuffer(pd3dDevice, context);
	}
	RenderText();
}
*/
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool collisionWithEdge(const Cylinder& cy, const Line& li, const SimpleMath::Vector3 & worldDistance, SimpleMath::Vector3 & impactPoint, SimpleMath::Vector3 & impactNormal, float & impactT)
{
	impactT = 1.0f;

	bool collision = false;

	auto worldDistanceWorldDistance = worldDistance.Dot(worldDistance);
	auto worldDistanceLength = sqrt(worldDistanceWorldDistance);

	SimpleMath::Vector2 projCylinderA = projectOnZ(SimpleMath::Vector3::Transform(cy.A, cy.ToCylinderLocalSpace));
	SimpleMath::Vector2 vProjLineX[] = { projectOnZ(SimpleMath::Vector3::Transform(li.A, cy.ToCylinderLocalSpace)), projectOnZ(SimpleMath::Vector3::Transform(li.B, cy.ToCylinderLocalSpace)) };
	SimpleMath::Vector2 projWorldDistance = projectOnZ(SimpleMath::Vector3::TransformNormal(worldDistance, cy.ToCylinderLocalSpace));
	SimpleMath::Vector2 projLineBA = projectOnZ(SimpleMath::Vector3::TransformNormal(li.BA, cy.ToCylinderLocalSpace));

	float last_t = 1.0f;

	//WITH EDGE(begin)////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//----Cylinder Ends Sphera's 
	{
		float t = 1.0f, a, b, c;

		bool rootFound = false;
		int lastRootFoundOnIndex;

		auto VA = worldDistance.Dot(li.BA);
		auto AA = li.BA.Dot(li.BA);
		auto VV = worldDistanceWorldDistance;

		a = VV - pow(VA, 2.0f) / AA;

		for (int i = 0; i < 2; i++)
		{
			auto S = cy.vX[i] - li.A;
			auto SA = S.Dot(li.BA);
			auto SS = S.Dot(S);
			auto SV = S.Dot(worldDistance);
			auto VA = worldDistance.Dot(li.BA);

			b = 2.0f*(SV - (SA*VA) / AA);
			c = SS - cy.cylinderRcylinderR - (SA*SA) / AA;

			if (getLowestRoot(a, b, c, t, &t))
			{
				rootFound = true;
				lastRootFoundOnIndex = i;
			}
		}

		if (rootFound)
		{
			auto cylinderX = cy.vX[lastRootFoundOnIndex];

			auto newCylinderX = cylinderX + t*worldDistance;

			auto l = (newCylinderX - li.A).Dot(li.BA) / li.BA.Dot(li.BA);

			if (.0f <= l&&l <= 1.0f)
			{
				auto lineX = li.A + l*li.BA;

				auto normalX = normalize(newCylinderX - lineX);

				if (lastRootFoundOnIndex == 0)
				{
					collision = cy.BA.Dot(normalX) >= .0f;
				}
				else
				{
					collision = cy.BA.Dot(normalX) <= .0f;
				}

				if (collision)
				{
					t = correctMistake(t, cy.r, vec4(normalX, -normalX.Dot(lineX)), cylinderX, newCylinderX);
					t = offsetBySmallDistantce(t, worldDistanceLength);

					impactNormal = normalX;
					impactPoint = lineX;
					impactT = last_t = t;
				}
			}
		}
	}
	//----Cylinder WITHOUT Ends Sphera's 
	//collide with line by self (2d)
	//check when |_d| near _epsilon, equation of line is degenerate, skip this
	if (projLineBA.Length() != 0.0f)
	{
		//make line equation
		SimpleMath::Vector3 pLine(projLineBA.y, -projLineBA.x, vProjLineX[0].y*projLineBA.x - vProjLineX[0].x*projLineBA.y);
		pLine *= (1.0 / sqrt(pLine.x*pLine.x + pLine.y*pLine.y));
		//swept circle into XZ
		auto lineDotS = pLine.Dot(SimpleMath::Vector3(projCylinderA.x, projCylinderA.y, 1.0f));
		auto lineDotV = pLine.Dot(SimpleMath::Vector3(projWorldDistance.x, projWorldDistance.y, 0.0f));
		//test sign of half space
		float sign = lineDotS < 0.0f ? -1.0f : 1.0f;

		float t = 1.0f;
		bool rootFound = false;
		SimpleMath::Vector2 skewLinesArgs;
		SimpleMath::Vector3 newCylinderA;

		//test if collision ocure
		if (abs(lineDotV) != 0.0f)
		{
			t = (cy.r * sign - lineDotS) / lineDotV;
			if (0.0f <= t&&t <= 1.0f)
			{
				newCylinderA = cy.A + t*worldDistance;

				skewLinesArgs = getDistanceBetweenSkewLines(newCylinderA, cy.BA, li.A, li.BA);

				//check only if impact possess line cut (with finit cylinder)
				if (.0f <= skewLinesArgs.x&&skewLinesArgs.x <= 1.0f && .0f <= skewLinesArgs.y&&skewLinesArgs.y <= 1.0f)
				{
					rootFound = true;
				}
			}
		}

		if (rootFound)
		{
			auto cylinderX = cy.A + skewLinesArgs.x * cy.BA;

			auto newCylinderX = newCylinderA + skewLinesArgs.x * cy.BA;

			auto lineX = li.A + skewLinesArgs.y * li.BA;

			auto normalX = normalize(newCylinderX - lineX);

			t = correctMistake(t, cy.r, vec4(normalX, -normalX.Dot(lineX)), cylinderX, newCylinderX);
			t = offsetBySmallDistantce(t, worldDistanceLength);

			if (t < last_t)
			{
				collision = true;

				impactNormal = normalX;
				impactPoint = lineX;
				impactT = t;
			}
		}
	}
	//WITH EDGE(end)//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	return collision;
}
bool collisionWithVertex(const Cylinder& cy, const SimpleMath::Vector3& ve, const SimpleMath::Vector3 & worldDistance, SimpleMath::Vector3 & impactPoint, SimpleMath::Vector3 & impactNormal, float & impactT)
{
	impactT = 1.0f;

	bool collision = false;

	auto worldDistanceWorldDistance = worldDistance.Dot(worldDistance);
	auto worldDistanceLength = sqrt(worldDistanceWorldDistance);

	SimpleMath::Vector2 projVe = projectOnZ(SimpleMath::Vector3::Transform(ve, cy.ToCylinderLocalSpace));

	SimpleMath::Vector2 projCylinderA = projectOnZ(SimpleMath::Vector3::Transform(cy.A, cy.ToCylinderLocalSpace));
	SimpleMath::Vector2 projWorldDistance = projectOnZ(SimpleMath::Vector3::TransformNormal(worldDistance, cy.ToCylinderLocalSpace));
	auto projWorldDistanceProjWorldDistance = projWorldDistance.Dot(projWorldDistance);

	float last_t = 1.0f;

	//WITH VERTEX(begin)//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//----Cylinder Ends Sphera's 
	//collide end spheres with line points (3d)
	{
		float t = 1, a, b, c;

		bool rootFound = false;
		int lastRootFoundOnIndex;

		a = worldDistanceWorldDistance;

		for (int i = 0; i < 2; i++)
		{
			auto d = cy.vX[i] - ve;

			b = 2.0f*worldDistance.Dot(d);
			c = d.Dot(d) - cy.cylinderRcylinderR;

			if (getLowestRoot(a, b, c, t, &t))
			{
				rootFound = true;
				lastRootFoundOnIndex = i;
			}
		}

		if (rootFound)
		{
			auto cylinderX = cy.vX[lastRootFoundOnIndex];

			auto newCylinderX = cylinderX + t*worldDistance;

			auto lineX = ve;

			auto normalX = normalize(newCylinderX - lineX);

			if (lastRootFoundOnIndex == 0)
			{
				collision = cy.BA.Dot(normalX) >= .0f;
			}
			else
			{
				collision = cy.BA.Dot(normalX) <= .0f;
			}

			if (collision)
			{
				t = correctMistake(t, cy.r, vec4(normalX, -normalX.Dot(lineX)), cylinderX, newCylinderX);
				t = offsetBySmallDistantce(t, worldDistanceLength);

				impactNormal = normalX;
				impactPoint = lineX;
				impactT = last_t = t;
			}
		}
	}
	//----Cylinder WITHOUT Ends Sphera's 
	//All to CylinderLocalSpace
	//project line to XZ , project distance to XZ , project cylinder.A to XZ
	//collide with line end points (2d)
	{
		float t = 1, a, b, c;

		bool rootFound = false;
		int lastRootFoundOnIndex;

		a = projWorldDistanceProjWorldDistance;

		{
			auto d = projCylinderA - projVe;

			b = 2.0f*projWorldDistance.Dot(d);
			c = d.Dot(d) - cy.cylinderRcylinderR;

			if (getLowestRoot(a, b, c, t, &t))
			{
				rootFound = true;
			}
		}

		if (rootFound)
		{
			auto newCylinderA = cy.A + t*worldDistance;

			auto lineX = ve;

			auto c = (lineX - newCylinderA).Dot(cy.BA) / cy.BA.Dot(cy.BA);

			if (.0f <= c&&c <= 1.0f)
			{
				auto cylinderX = cy.A + c * cy.BA;

				auto newCylinderX = newCylinderA + c * cy.BA;

				auto normalX = normalize(newCylinderX - lineX);

				t = correctMistake(t, cy.r, vec4(normalX, -normalX.Dot(lineX)), cylinderX, newCylinderX);
				t = offsetBySmallDistantce(t, worldDistanceLength);

				if (t < last_t)
				{
					collision = true;

					impactNormal = normalX;
					impactPoint = lineX;
					impactT = t;
				}
			}
		}
	}
	//WITH VERTEX(end)///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	return collision;
}
bool collisionWithFace(const Cylinder & cylinder, const SimpleMath::Matrix& faceBasis, const SimpleMath::Vector2& faceHalfSize, const SimpleMath::Vector3 & worldDistance, SimpleMath::Vector3 & impactPoint, SimpleMath::Vector3 & impactNormal, float & impactT){
	impactT = 1.0f;

	bool collision = false;

	SimpleMath::Vector4 pFace = vec4(faceBasis.Backward(), -faceBasis.Backward().Dot(faceBasis.Translation()));
	float vFaceDotS[2] = { pFace.Dot(vec4(cylinder.A, 1.0f)), pFace.Dot(vec4(cylinder.B, 1.0f)) };

	{

		if (vFaceDotS[0] > cylinder.r && vFaceDotS[1] > cylinder.r)
		{
			float t = 1.0f;
			int index = 0;

			bool rootFound = false;

			auto faceDotV = pFace.Dot(vec4(worldDistance, 0.0f));

			for (int i = 0; i < 2; i++)
			{
				if (abs(faceDotV) != 0.0f)
				{
					auto faceDotS = vFaceDotS[i];

					float last_t = (cylinder.r - faceDotS) / faceDotV;

					if (0.0f <= last_t && last_t <= t)
					{
						rootFound = true;
						t = last_t;
						index = i;
					}
				}
			}

			if (rootFound)
			{
				SimpleMath::Vector3 vCylinderX[2] = { cylinder.A, cylinder.B };

				auto worldDistanceLength = worldDistance.Length();

				auto cylinderX = vCylinderX[index];

				auto newCylinderX = cylinderX + t*worldDistance;

				auto relNewCylinderX = newCylinderX - faceBasis.Translation();

				auto x_coord = faceBasis.Right().Dot(relNewCylinderX);

				auto y_coord = faceBasis.Up().Dot(relNewCylinderX);

				if (abs(x_coord) <= faceHalfSize.x)
				{
					if (abs(y_coord) <= faceHalfSize.y)
					{
						collision = true;

						t = correctMistake(t, cylinder.r, pFace, cylinderX, newCylinderX);
						t = offsetBySmallDistantce(t, worldDistanceLength);

						impactNormal = faceBasis.Backward();
						impactPoint = x_coord*faceBasis.Right() + y_coord*faceBasis.Up() + faceBasis.Translation();
						impactT = t;
					}
				}
			}
		}
	}

	return collision;
}
bool collisionWithBox(const Cylinder & cylinder, const SimpleMath::Matrix& boxBasis, const SimpleMath::Vector3& boxHalfSize, const SimpleMath::Vector3 & worldDistance, SimpleMath::Vector3 & impactPoint, SimpleMath::Vector3 & impactNormal, float & impactT)
{
	impactT = 1.0f;

	bool _collison = false;

	SimpleMath::Vector3 _impactPoint;
	SimpleMath::Vector3 _impactNormal;
	float _t = 1.0f;

	bool lastCollison = false;

	SimpleMath::Vector3 lastImpactPoint;
	SimpleMath::Vector3 lastImpactNormal;
	float last_t;

	SimpleMath::Vector3 vFaceNormal[] = { boxBasis.Right(), boxBasis.Up(), boxBasis.Backward() };
	float vBoxHalfSize[] = { boxHalfSize.x, boxHalfSize.y, boxHalfSize.z };

	for (int i = 0; i < 3; i++)
	{
		auto faceNormal = vFaceNormal[i];
		auto faceOffset = vBoxHalfSize[i];

		SimpleMath::Vector3 vFaceTB[] = { vFaceNormal[(i + 1) % 3], vFaceNormal[(i + 2) % 3] };
		float vFaceHalfSize[] = { vBoxHalfSize[(i + 1) % 3], vBoxHalfSize[(i + 2) % 3] };

		for (int j = 0; j < 2; j++)
		{
			faceNormal *= -1.0f;

			if (faceNormal.Dot(worldDistance) < .0f)
			{
				auto faceOrigin = boxBasis.Translation() + faceOffset * faceNormal;

				{
					lastCollison = false;
				}

				{
					SimpleMath::Matrix faceBasis(
						vec4(vFaceTB[0], 0),
						vec4(vFaceTB[1], 0),
						vec4(faceNormal, 0),
						vec4(faceOrigin, 1)
						);

					SimpleMath::Vector2 faceHalfSize(
						vFaceHalfSize[0],
						vFaceHalfSize[1]
						);

					if (collisionWithFace(cylinder, faceBasis, faceHalfSize, worldDistance, lastImpactPoint, lastImpactNormal, last_t))
					{
						if (last_t < _t)
						{
							lastCollison = true;

							_collison = true;
							_impactPoint = lastImpactPoint;
							_impactNormal = faceNormal;
							_t = last_t;
						}
					}
				}

				if (!lastCollison)
				{
					SimpleMath::Vector3 edgeNormal;

					for (int k = 0; k < 2; k++)
					{
						auto baseLineA = faceOrigin + vFaceHalfSize[k % 2] * vFaceTB[k % 2];
						auto baseLineB = faceOrigin - vFaceHalfSize[k % 2] * vFaceTB[k % 2];

						auto lineOffset = vFaceHalfSize[(k + 1) % 2] * vFaceTB[(k + 1) % 2];

						for (int l = 0; l < 2; l++)
						{
							lineOffset *= -1.0f;

							auto lineA = baseLineA + lineOffset;
							auto lineB = baseLineB + lineOffset;

							Line line(lineA.x, lineA.y, lineA.z, lineB.x, lineB.y, lineB.z);

							if (collisionWithEdge(cylinder, line, worldDistance, lastImpactPoint, lastImpactNormal, last_t))
							{
								if (last_t < _t)
								{
									{
										auto midPoint = 0.5f*(line.A + line.B);
										auto tangent0 = midPoint - faceOrigin;
										tangent0.Normalize();
										if ((line.A - lastImpactPoint).LengthSquared() < 0.001f || (line.B - lastImpactPoint).LengthSquared() < 0.001f)
										{
											auto tangent1 = lastImpactPoint - midPoint;
											tangent1.Normalize();
											edgeNormal = (faceNormal + tangent0 + tangent1);
											edgeNormal.Normalize();
										}
										else
										{
											edgeNormal = (faceNormal + tangent0);
											edgeNormal.Normalize();
										}
									}
									lastCollison = true;

									_collison = true;
									_impactPoint = lastImpactPoint;
									_impactNormal = edgeNormal;
									_t = last_t;
								}
							}
						}
					}
				}

				if (!lastCollison)
				{
					SimpleMath::Vector3 vertexNormal;

					auto baseLineA = faceOrigin + vFaceHalfSize[0] * vFaceTB[0];
					auto baseLineB = faceOrigin - vFaceHalfSize[0] * vFaceTB[0];

					auto lineOffset = vFaceHalfSize[1] * vFaceTB[1];

					for (int l = 0; l < 2; l++)
					{
						lineOffset *= -1.0f;

						SimpleMath::Vector3 lineX[] = { baseLineA + lineOffset, baseLineB + lineOffset };

						for (int k = 0; k < 2; k++)
						{
							if (collisionWithVertex(cylinder, lineX[k], worldDistance, lastImpactPoint, lastImpactNormal, last_t))
							{
								if (last_t < _t)
								{
									{
										auto midPoint = 0.5f*(lineX[0] + lineX[1]);
										auto tangent0 = midPoint - faceOrigin;
										tangent0.Normalize();
										auto tangent1 = lastImpactPoint - midPoint;
										tangent1.Normalize();
										vertexNormal = (faceNormal + tangent0 + tangent1);
										vertexNormal.Normalize();
									}
									lastCollison = true;

									_collison = true;
									_impactPoint = lastImpactPoint;
									_impactNormal = vertexNormal;
									_t = last_t;
								}
							}
						}
					}
				}
			}
		}
	}

	if (_collison)
	{
		impactPoint = _impactPoint;
		impactNormal = _impactNormal;
		impactT = _t;
	}

	return _collison;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

SimpleMath::Vector3 __impactPoint;
SimpleMath::Vector3 __impactNormal;
SimpleMath::Vector3 __capsuleDirecion;
bool globalCollision = false;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int collisionShapesCount()
{
	return 1;
}
SimpleMath::Matrix GetBoxBasis()
{
	return
		SimpleMath::Matrix::CreateRotationX(0.523599)*
		SimpleMath::Matrix::CreateRotationY(0.261799)*
		SimpleMath::Matrix::CreateRotationY(0.698132)*
		SimpleMath::Matrix(
			vec4(SimpleMath::Vector3(1, 0, 0), 0),
			vec4(SimpleMath::Vector3(0, 1, 0), 0),
			vec4(SimpleMath::Vector3(0, 0, 1), 0),
			vec4(0.5f*(SimpleMath::Vector3(50, 2, 8) + SimpleMath::Vector3(11, 0, 50)), 1)
		);
}
void physCollision(const SimpleMath::Vector3 & worldDistance, SimpleMath::Vector3 & impactPoint, SimpleMath::Vector3 & impactNormal, float & t)
{
	auto boxBasis = GetBoxBasis();

	SimpleMath::Vector3 boxHalfSize(10, 10, 10);

	collisionWithBox(Cylinder(cylinder.A.x, cylinder.A.y, cylinder.A.z, cylinder.B.x, cylinder.B.y, cylinder.B.z), boxBasis, boxHalfSize, worldDistance, impactPoint, impactNormal, t);
}
void physCollision(bool sweep, const SimpleMath::Vector3 & worldDistance, SimpleMath::Vector3 & impactPoint, SimpleMath::Vector3 & impactNormal, SimpleMath::Vector3 & direction)
{
	direction = worldDistance;
	direction.Normalize();

	float min_t = 1.0f;

	physCollision(worldDistance, impactPoint, impactNormal, min_t);

	cylinder.A += min_t*worldDistance;
	cylinder.B += min_t*worldDistance;

	if (!sweep)
	{
		return;
	}

	{
		float remaining_t = 1.0 - min_t;

		if (remaining_t > 0.0f)
		{
			auto perpWorldDir = worldDistance - worldDistance.Dot(impactNormal) * impactNormal;
			perpWorldDir.Normalize();

			auto remainingWorldDistance = worldDistance.Length() * perpWorldDir;

			remaining_t = 1.0f;

			physCollision(remainingWorldDistance, impactPoint, impactNormal, remaining_t);

			cylinder.A += remaining_t*remainingWorldDistance;
			cylinder.B += remaining_t*remainingWorldDistance;
		}
	}
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void set_point_direction(DirectX::XMFLOAT4 point, DirectX::XMFLOAT4 direction, DirectX::XMFLOAT4 color);

void renderScene(ID3D11Device* pd3dDevice, ID3D11DeviceContext* context, float fElapsedTime, IEffect* effect, bool renderIntoShadowMap){
	float epsilon = 0.001f;
	SimpleMath::Matrix ToCylinderLocalSpace;
	auto cylinderBA = cylinder.B - cylinder.A;
	auto lineBA = line.B - line.A;
	{
		// std make frame
		// trivial impl
		SimpleMath::Vector3 X(1, 0, 0);
		auto Y = cylinderBA;
		Y.Normalize();

		// check X collinearity
		if (1.0f - abs(X.Dot(Y)) < epsilon)
		{
			X = SimpleMath::Vector3(0, 1, 0);
		}

		auto Z = X.Cross(Y);
		X = Y.Cross(Z);

		X.Normalize();
		Y.Normalize();
		Z.Normalize();
		// Cylinder it is Y
		ToCylinderLocalSpace = SimpleMath::Matrix(X, Y, Z);
		ToCylinderLocalSpace = ToCylinderLocalSpace.Transpose();
	}
	auto boxBasis = GetBoxBasis();
	SimpleMath::Vector3 boxHalfSize(10, 10, 10);
	auto worldDistance = velocity*fElapsedTime;
	if (!renderIntoShadowMap && worldDistance.Length() > 0.0f)
	{
		//collision = collisionWithEdge(cylinder, line, worldDistance, impactPoint);
		physCollision(true, worldDistance, __impactPoint, __impactNormal, __capsuleDirecion);
	}
	if (forcePush)
	{
		//physCollision(true, worldDistance, impactPoint, impactNormal, capsuleDirecion);
		//forcePush = false;
	}
	{
		set_scene_world_matrix(SimpleMath::Matrix::CreateTranslation(__impactPoint));
		set_scene_constant_buffer(context);

		G->sphere_model2->Draw(G->model_green_wireframe_effect.get(), G->cylinder_model_input_layout.Get(), false, false, [=]{
			context->OMSetBlendState(G->render_states->Opaque(), Colors::Black, 0xFFFFFFFF);
			context->RSSetState(G->render_states->Wireframe());
			context->OMSetDepthStencilState(G->render_states->DepthDefault(), 0);
		});
	}
	{
		auto H = cylinder.A - cylinder.B;
		set_scene_world_matrix(SimpleMath::Matrix::CreateScale(cylinder.r, H.Length(), cylinder.r) * SimpleMath::Matrix::CreateTranslation(0, 0.5f*H.Length(), 0) * ToCylinderLocalSpace.Transpose() * SimpleMath::Matrix::CreateTranslation(cylinder.A));
		set_scene_constant_buffer(context);

		G->cylinder_model->Draw(G->model_wite_wireframe_effect.get(), G->cylinder_model_input_layout.Get(), false, false, [=]{
			context->OMSetBlendState(G->render_states->Opaque(), Colors::Black, 0xFFFFFFFF);
			context->RSSetState(G->render_states->Wireframe());
			context->OMSetDepthStencilState(G->render_states->DepthDefault(), 0);
		});

		set_scene_world_matrix(SimpleMath::Matrix::CreateScale(cylinder.r * 4, cylinder.r * 4, cylinder.r * 4) * SimpleMath::Matrix::CreateTranslation(cylinder.A));
		set_scene_constant_buffer(context);

		G->sphere_model2->Draw(G->model_green_wireframe_effect.get(), G->cylinder_model_input_layout.Get(), false, false, [=]{
			context->OMSetBlendState(G->render_states->Opaque(), Colors::Black, 0xFFFFFFFF);
			context->RSSetState(G->render_states->Wireframe());
			context->OMSetDepthStencilState(G->render_states->DepthDefault(), 0);
		});

		set_scene_world_matrix(SimpleMath::Matrix::CreateScale(cylinder.r * 4, cylinder.r * 4, cylinder.r * 4) * SimpleMath::Matrix::CreateTranslation(cylinder.B));
		set_scene_constant_buffer(context);

		G->sphere_model2->Draw(G->model_green_wireframe_effect.get(), G->cylinder_model_input_layout.Get(), false, false, [=]{
			context->OMSetBlendState(G->render_states->Opaque(), Colors::Black, 0xFFFFFFFF);
			context->RSSetState(G->render_states->Wireframe());
			context->OMSetDepthStencilState(G->render_states->DepthDefault(), 0);
		});
	}
	{
		{
			// std make frame
			// check X collinearity omitted
			// trivial impl
			SimpleMath::Vector3 X(1, 0, 0);
			auto Y = line.B - line.A;
			auto Z = X.Cross(Y);
			X = Y.Cross(Z);
			X.Normalize();
			Y.Normalize();
			Z.Normalize();
			// Cylinder it is Y
			auto FromLineLocalSpace = SimpleMath::Matrix(X, Y, Z);

			//set_scene_world_matrix(SimpleMath::Matrix::CreateScale(1, 150, 1) * FromLineLocalSpace * SimpleMath::Matrix::CreateTranslation(line.A));
			//set_scene_constant_buffer(context);

			//G->cylinder_model->Draw(G->model_wite_wireframe_effect.get(), G->cylinder_model_input_layout.Get(), false, false, [=]{
			//	context->OMSetBlendState(G->render_states->Opaque(), Colors::Black, 0xFFFFFFFF);
			//	context->RSSetState(G->render_states->Wireframe());
			//	context->OMSetDepthStencilState(G->render_states->DepthDefault(), 0);
			//});

			context->GSSetConstantBuffers(0, 1, constantBuffersToArray(*(G->scene_constant_buffer)));
			context->PSSetConstantBuffers(0, 1, constantBuffersToArray(*(G->scene_constant_buffer)));

			set_scene_world_matrix(SimpleMath::Matrix::Identity);
			if (__impactPoint.Length() > 0.001)
			{
				set_point_direction(vec4(__impactPoint, 1.0f), vec4(__capsuleDirecion, 0.0f), SimpleMath::Vector4(0, 0, 1, 0));
			}
			else
			{
				set_point_direction(vec4(cylinder.r * __capsuleDirecion + 0.5f*(cylinder.A + cylinder.B), 1.0f), vec4(__capsuleDirecion, 0.0f), SimpleMath::Vector4(0, 0, 1, 0));
			}
			set_scene_constant_buffer(context);

			ortho_box_draw(context, G->red_line_effect.get(), 0, [=]{
				context->OMSetBlendState(G->render_states->Opaque(), Colors::Black, 0xFFFFFFFF);
				context->RSSetState(G->render_states->Wireframe());
				context->OMSetDepthStencilState(G->render_states->DepthDefault(), 0);
			});

			if (__impactPoint.Length() > 0.001)
			{
				set_scene_world_matrix(SimpleMath::Matrix::Identity);
				set_point_direction(vec4(__impactPoint, 1.0f), vec4(__impactNormal, 0.0f), SimpleMath::Vector4(1, 0, 0, 0));
				set_scene_constant_buffer(context);

				ortho_box_draw(context, G->red_line_effect.get(), 0, [=]{
					context->OMSetBlendState(G->render_states->Opaque(), Colors::Black, 0xFFFFFFFF);
					context->RSSetState(G->render_states->Wireframe());
					context->OMSetDepthStencilState(G->render_states->DepthDefault(), 0);
				});
			}
		}
	}
	{
		context->GSSetConstantBuffers(0, 1, constantBuffersToArray(*(G->scene_constant_buffer)));

		ortho_box_set_world_matrix2(SimpleMath::Matrix::CreateScale(2.0f*boxHalfSize) * boxBasis);

		set_scene_constant_buffer(context);

		ortho_box_draw(context, G->ortho_box_effect.get(), 0, [=]{
			context->OMSetBlendState(G->render_states->Opaque(), Colors::Black, 0xFFFFFFFF);
			context->RSSetState(G->render_states->Wireframe());
			context->OMSetDepthStencilState(G->render_states->DepthDefault(), 0);
		});
	}
	{
		using namespace SimpleMath;
		Vector3 A1(5,10,11), B1(50,12,13);
		Vector3 A2(17,2,8), B2(11,0,50);

		auto ret = getDistanceBetweenSkewLines(A1, B1 - A1, A2, B2 - A2);
		auto C1 = A1 + ret.x*(B1 - A1);
		auto D1 = A2 + ret.y*(B2 - A2);

		ret = getDistanceBetweenSkewLines(A1 + Vector3(5, 85, 20), B1 - A1, A2, B2 - A2);
		auto E1 = A1 + ret.x*(B1 - A1);
		auto F1 = A2 + ret.y*(B2 - A2);

		auto t1 = D1 - C1; t1.Normalize();
		auto t2 = F1 - E1; t2.Normalize();

		auto test = t1.Dot(t2);

		context->PSSetConstantBuffers(0, 1, constantBuffersToArray(*(G->scene_constant_buffer)));
		//SimpleMath::
	}

	context->PSSetConstantBuffers(0, 1, constantBuffersToArray(*(G->scene_constant_buffer)));
	context->VSSetConstantBuffers(0, 1, constantBuffersToArray(*(G->scene_constant_buffer)));

	context->PSSetSamplers(0, 2, samplerStateToArray(G->render_states->AnisotropicClamp(), G->render_states->AnisotropicWrap()));

	/*
	sphere_set_world_matrix();

	set_scene_constant_buffer(context);

	sphere_draw(context, effect, G->model_input_layout.Get(), [=]{
		context->PSSetShaderResources(0, 1, shaderResourceViewToArray(G->ice_texture.Get()));

		context->OMSetBlendState(G->render_states->Opaque(), Colors::Black, 0xFFFFFFFF);
		context->RSSetState(G->render_states->CullCounterClockwise());
		context->OMSetDepthStencilState(G->render_states->DepthDefault(), 0);
	});

	cube_set_world_matrix();

	set_scene_constant_buffer(context);

	cube_draw(context, effect, G->model_input_layout.Get(), [=]{
		context->PSSetShaderResources(0, 1, shaderResourceViewToArray(G->wall_texture.Get()));

		context->OMSetBlendState(G->render_states->Opaque(), Colors::Black, 0xFFFFFFFF);
		context->RSSetState(G->render_states->CullCounterClockwise());
		context->OMSetDepthStencilState(G->render_states->DepthDefault(), 0);
	});
	*/

	tree_set_world_matrix();

	set_scene_constant_buffer(context);
	/*
	tree_trunk_draw(context, effect, G->model_input_layout.Get(), [=]{
		context->PSSetShaderResources(0, 1, shaderResourceViewToArray(G->trunk_texture.Get()));

		context->OMSetBlendState(G->render_states->Opaque(), Colors::Black, 0xFFFFFFFF);
		context->RSSetState(G->render_states->CullCounterClockwise());
		context->OMSetDepthStencilState(G->render_states->DepthDefault(), 0);
	});

	tree_leaf_draw(context, effect, G->model_input_layout.Get(), [=]{
		context->PSSetShaderResources(0, 1, shaderResourceViewToArray(G->leaf_texture.Get()));

		context->OMSetBlendState(renderIntoShadowMap ? G->render_states->Opaque() : G->m_pBlendState.Get(), Colors::Black, 0xFFFFFFFF);
		context->RSSetState(G->render_states->CullNone());
		context->OMSetDepthStencilState(G->render_states->DepthDefault(), 0);
	});
	*/
	plane_set_world_matrix();

	set_scene_constant_buffer(context);

	plane_draw(context, effect, G->model_input_layout.Get(), [=]{
		context->PSSetShaderResources(0, 1, shaderResourceViewToArray(G->metal_texture.Get()));

		context->OMSetBlendState(G->render_states->Opaque(), Colors::Black, 0xFFFFFFFF);
		context->RSSetState(G->render_states->CullCounterClockwise());
		context->OMSetDepthStencilState(G->render_states->DepthDefault(), 0);
	});
}
/*
void postProccessBlur(ID3D11Device* pd3dDevice, ID3D11DeviceContext* context, _In_opt_ std::function<void __cdecl()> setHState, _In_opt_ std::function<void __cdecl()> setVState){
	/////
	context->PSSetConstantBuffers(0, 2, constantBuffersToArray(*(G->scene_constant_buffer), *(G->blur_constant_buffer)));
	/////

	set_scene_world_matrix(SimpleMath::Matrix::Identity);

	set_scene_constant_buffer(context);

	setHState();

	post_proccess(context, G->blur_h_effect.get(), 0, [=]{
		context->OMSetBlendState(G->render_states->Opaque(), Colors::White, 0xFFFFFFFF);
		context->RSSetState(G->render_states->CullNone());
		context->OMSetDepthStencilState(G->render_states->DepthNone(), 0);
	});

	setVState();

	post_proccess(context, G->blur_v_effect.get(), 0, [=]{
		context->OMSetBlendState(G->render_states->Opaque(), Colors::White, 0xFFFFFFFF);
		context->RSSetState(G->render_states->CullNone());
		context->OMSetDepthStencilState(G->render_states->DepthNone(), 0);
	});

	context->PSSetConstantBuffers(0, 2, constantBuffersToArray(0, 0));
}
*/
/*
void postProccessGBuffer(ID3D11Device* pd3dDevice, ID3D11DeviceContext* context){
	/////
	context->PSSetConstantBuffers(0, 1, constantBuffersToArray(*(G->scene_constant_buffer)));
	/////

	set_scene_world_matrix(SimpleMath::Matrix::Identity);

	set_scene_constant_buffer(context);

	post_proccess(context, G->quad_effect.get(), G->quad_mesh_layout.Get(), [=]{
		context->PSSetShaderResources(0, 3, shaderResourceViewToArray(
			SCG->colorSRV.Get(),
			SCG->normalSRV.Get()
			//SCG->depthStencilSRV.Get()
			)
		);

		context->OMSetBlendState(G->render_states->Opaque(), Colors::White, 0xFFFFFFFF);
		context->RSSetState(G->render_states->CullNone());
		context->OMSetDepthStencilState(G->render_states->DepthNone(), 0);
	});
}
*/
/*
void renderSceneIntoGBuffer(ID3D11Device* pd3dDevice, ID3D11DeviceContext* context)
{
	/////
	context->VSSetConstantBuffers(0, 1, constantBuffersToArray(*(G->scene_constant_buffer)));
	
	context->PSSetSamplers(0, 1, samplerStateToArray(G->render_states->AnisotropicWrap()));
	/////

	ground_set_world_matrix();

	set_scene_constant_buffer(context);

	ground_draw(context, G->model_effect.get(), G->model_input_layout.Get(), [=]{
		context->PSSetShaderResources(0, 1, shaderResourceViewToArray(G->ground_texture.Get()));

		context->OMSetBlendState(G->render_states->Opaque(), Colors::Black, 0xFFFFFFFF);
		context->RSSetState(G->render_states->CullCounterClockwise());
		context->OMSetDepthStencilState(G->render_states->DepthDefault(), 0);
	});

	wall_set_world_matrix();

	set_scene_constant_buffer(context);

	wall_draw(context, G->model_effect.get(), G->model_input_layout.Get(), [=]{
		context->PSSetShaderResources(0, 1, shaderResourceViewToArray(G->wall_texture.Get()));

		context->OMSetBlendState(G->render_states->Opaque(), Colors::Black, 0xFFFFFFFF);
		context->RSSetState(G->render_states->CullCounterClockwise());
		context->OMSetDepthStencilState(G->render_states->DepthDefault(), 0);
	});
}
*/
