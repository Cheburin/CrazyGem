#include "main.h"

extern GraphicResources * G;

extern SceneState scene_state;

using namespace SimpleMath;

void loadMatrix_VP(Matrix & v, Matrix & p){
	v = Matrix(scene_state.mView).Transpose();
	p = Matrix(scene_state.mProjection).Transpose();
}
void loadMatrix_WP(Matrix & w, Matrix & p){
	w = Matrix(scene_state.mWorld).Transpose();
	p = Matrix(scene_state.mProjection).Transpose();
}
void storeMatrix(Matrix & w, Matrix & wv, Matrix & wvp, Matrix & wvp1, Matrix & wvp2){
	scene_state.mWorld = w.Transpose();
	scene_state.mWorldView = wv.Transpose();
	scene_state.mWorldViewProjection = wvp.Transpose();
	scene_state.mObject1WorldViewProjection = wvp1.Transpose();
	scene_state.mObject2WorldViewProjection = wvp2.Transpose();
}

void DrawQuad(ID3D11DeviceContext* pd3dImmediateContext, _In_ IEffect* effect,
	_In_opt_ std::function<void __cdecl()> setCustomState){
	effect->Apply(pd3dImmediateContext);
	setCustomState();

	pd3dImmediateContext->IASetInputLayout(nullptr);
	pd3dImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);// D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	pd3dImmediateContext->Draw(1, 0);
}

void set_point_direction(DirectX::XMFLOAT4 point, DirectX::XMFLOAT4 direction, DirectX::XMFLOAT4 color){
	scene_state.vPoint = point;
	scene_state.vDirection = direction;
	scene_state.vColor = color;
}

void set_scene_world_matrix(DirectX::XMFLOAT4X4 transformation){

	Matrix w, v, p;
	loadMatrix_VP(v, p);

	w = transformation;// .Translation(Vector3(0.0f, 6.0f, 8.0f));
	Matrix  wv = w * v;
	Matrix wvp = wv * p;

	storeMatrix(w, wv, wvp, Matrix(), Matrix());
};
void scene_draw(ID3D11DeviceContext* pd3dImmediateContext, IEffect* effect, ID3D11InputLayout* inputLayout, _In_opt_ std::function<void(ID3D11ShaderResourceView * texture, DirectX::XMFLOAT4X4 transformation)> setCustomState){
	//G->scene->draw(pd3dImmediateContext, effect, inputLayout, setCustomState);
}

void post_proccess(ID3D11DeviceContext* context, IEffect* effect, ID3D11InputLayout* inputLayout, _In_opt_ std::function<void __cdecl()> setCustomState){
	/*
	context->IASetInputLayout(inputLayout);

	effect->Apply(context);

	auto vertexBuffer = G->single_point_buffer.Get();
	UINT vertexStride = sizeof(XMFLOAT3);
	UINT vertexOffset = 0;

	context->IASetVertexBuffers(0, 1, &vertexBuffer, &vertexStride, &vertexOffset);

    setCustomState();

	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);
	context->Draw(1, 0);
	*/
	DrawQuad(context, effect, setCustomState);

	//G->quad_mesh->Draw(context, effect, inputLayout, [=]
	//{
	//	setCustomState();
	//});
}
extern DirectX::XMFLOAT4 lightDirection;
extern DirectX::XMFLOAT3 lightPosition;
extern DirectX::XMFLOAT3 lightLookAt;
extern DirectX::XMFLOAT4X4    lightViewProjection;
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void sphere_set_world_matrix(){
	Matrix w, v, p;
	loadMatrix_VP(v, p);

	w.Translation(Vector3(2.0f, 2.0f, 0.0f));
	Matrix  wv = w * v;
	Matrix wvp = wv * p;

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	auto wvp1 = Matrix(XMMatrixLookAtLH(Vector3(10 * 0.707, 10 * 0.707, 0), Vector3(0.0f, 0.0f, 0.0f), Vector3(0.0f, 1.0f, 0.0f)));
	wvp1 = w * wvp1 * Matrix(lightViewProjection);
	auto wvp2 = Matrix(XMMatrixLookAtLH(Vector3(-5.0f, 8.0f, -5.0f), Vector3(0.0f, 0.0f, 0.0f), Vector3(0.0f, 1.0f, 0.0f)));
	wvp2 = w * wvp2 * Matrix(lightViewProjection);
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	storeMatrix(w, wv, wvp, wvp1, wvp2);
}
void sphere_draw(ID3D11DeviceContext* pd3dImmediateContext, IEffect* effect, ID3D11InputLayout* inputLayout, _In_opt_ std::function<void __cdecl()> setCustomState){
	G->sphere_model->Draw(pd3dImmediateContext, effect, inputLayout, [=]
	{
		setCustomState();
	});
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void plane_set_world_matrix(){
	Matrix w, wv, wvp, v, p, wvp2;
	loadMatrix_VP(v, p);

	w = Matrix::CreateScale(4, 2.4, 2.4) * Matrix::CreateTranslation(0, 1, 0);
	wv = w * v;
	wvp = wv * p;

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	auto wvp1 = w * Matrix(XMMatrixLookAtLH(Vector3(lightPosition), Vector3(lightLookAt), Vector3(0.0f, 1.0f, 0.0f))) * Matrix(lightViewProjection);
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	scene_state.viewLightPos = lightDirection;

	storeMatrix(w, wv, wvp, wvp1, wvp2);
}
void plane_draw(ID3D11DeviceContext* pd3dImmediateContext, IEffect* effect, ID3D11InputLayout* inputLayout, _In_opt_ std::function<void __cdecl()> setCustomState){
	G->plane_model->Draw(pd3dImmediateContext, effect, inputLayout, [=]
	{
		setCustomState();
	});
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void cube_set_world_matrix(){
	Matrix w, v, p;
	loadMatrix_VP(v, p);

	w.Translation(Vector3(-2.0f, 2.0f, 0.0f));
	Matrix  wv = w * v;
	Matrix wvp = wv * p;

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	auto wvp1 = Matrix(XMMatrixLookAtLH(Vector3(10 * 0.707, 10 * 0.707, 0), Vector3(0.0f, 0.0f, 0.0f), Vector3(0.0f, 1.0f, 0.0f)));
	//auto wvp1 = Matrix(XMMatrixLookAtLH(Vector3(2.0f, 5.0f, -2.0f), Vector3(0.0f, 0.0f, 0.0f), Vector3(0.0f, 1.0f, 0.0f)));
	wvp1 = w * wvp1 * Matrix(lightViewProjection); //проекцию делаем в шейдере
	auto wvp2 = Matrix(XMMatrixLookAtLH(Vector3(-5.0f, 8.0f, -5.0f), Vector3(0.0f, 0.0f, 0.0f), Vector3(0.0f, 1.0f, 0.0f)));
	wvp2 = w * wvp2 * Matrix(lightViewProjection);
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	storeMatrix(w, wv, wvp, wvp1, wvp2);
}
void cube_draw(ID3D11DeviceContext* pd3dImmediateContext, IEffect* effect, ID3D11InputLayout* inputLayout, _In_opt_ std::function<void __cdecl()> setCustomState){
	G->cube_model->Draw(pd3dImmediateContext, effect, inputLayout, [=]
	{
		setCustomState();
	});
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void tree_set_world_matrix(){
	Matrix w, wv, wvp, v, p, wvp2;
	loadMatrix_VP(v, p);

	w = Matrix::CreateScale(0.1, 0.1, 0.1) * Matrix::CreateTranslation(0, 1.0, 0);
	wv = w * v;
	wvp = wv * p;

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	auto wvp1 = w * Matrix(XMMatrixLookAtLH(Vector3(lightPosition), Vector3(lightLookAt), Vector3(0.0f, 1.0f, 0.0f))) * Matrix(lightViewProjection);
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	scene_state.viewLightPos = lightDirection;

	storeMatrix(w, wv, wvp, wvp1, wvp2);
}
void tree_trunk_draw(ID3D11DeviceContext* pd3dImmediateContext, IEffect* effect, ID3D11InputLayout* inputLayout, _In_opt_ std::function<void __cdecl()> setCustomState){
	G->trees_trunk_model->Draw(pd3dImmediateContext, effect, inputLayout, [=]
	{
		setCustomState();
	});
}
void tree_leaf_draw(ID3D11DeviceContext* pd3dImmediateContext, IEffect* effect, ID3D11InputLayout* inputLayout, _In_opt_ std::function<void __cdecl()> setCustomState){
	G->trees_leaf_model->Draw(pd3dImmediateContext, effect, inputLayout, [=]
	{
		setCustomState();
	});
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ortho_box_set_world_matrix(){
	Matrix  v, p, wvp1, wvp2;
	loadMatrix_VP(v, p);

	//lightPosition = Vector3(0, 10, 0);
	//lightLookAt = Vector3(10, 0, 10);

	auto origin = Vector3(lightPosition);

	auto zDirection = Vector3(lightLookAt) - Vector3(lightPosition);
	zDirection.Normalize();
	auto xDirection = Vector3(0.0f, 1.0f, 0.0f).Cross(zDirection);
	xDirection.Normalize();
	auto yDirection = zDirection.Cross(xDirection);
	yDirection.Normalize();
	
	Matrix  w = Matrix::CreateScale(15, 15, 499) *
		Matrix::CreateTranslation(Vector3(0, 0, 499 / 2.0)) *
		Matrix(xDirection, yDirection, zDirection) *
		Matrix::CreateTranslation(origin);
	Matrix  wv = w * v;
	Matrix wvp = wv * p;

	storeMatrix(w, wv, wvp, wvp1, wvp2);
}

void ortho_box_set_world_matrix2(Matrix w){
	Matrix  v, p, wvp1, wvp2;
	loadMatrix_VP(v, p);

	//lightPosition = Vector3(0, 10, 0);
	//lightLookAt = Vector3(10, 0, 10);

	Matrix  wv = w * v;
	Matrix wvp = wv * p;

	storeMatrix(w, wv, wvp, wvp1, wvp2);
}

void  ortho_box_draw(ID3D11DeviceContext* pd3dImmediateContext, IEffect* effect, ID3D11InputLayout* inputLayout, _In_opt_ std::function<void __cdecl()> setCustomState){
	DrawQuad(pd3dImmediateContext, effect, [=]{
		setCustomState();
	});
}