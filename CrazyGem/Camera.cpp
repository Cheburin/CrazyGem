#include "main.h"

float m_pitch;
float m_yaw = -XM_PI / 2.0f;

const float ROTATION_GAIN = 0.004f;
const float MOVEMENT_GAIN = 0.02f;

DirectX::XMFLOAT3 m_cameraPos = XMFLOAT3(5.0f, 8.0f, -5.0f);

extern std::unique_ptr<Keyboard> _keyboard;
extern std::unique_ptr<Mouse> _mouse;

extern SceneState scene_state;

bool fire = false;
bool forcePush = false;
extern SimpleMath::Vector3 velocity;
extern bool globalCollision;
namespace Camera{
	void CALLBACK OnFrameMove(double fTime, float fElapsedTime, void* pUserContext)
	{
		// Update the camera's position based on user input 
		auto mouse = _mouse->GetState();

		if (mouse.positionMode == Mouse::MODE_RELATIVE)
		{
			SimpleMath::Vector3 delta = SimpleMath::Vector3(float(mouse.x), float(mouse.y), 0.f)
				* ROTATION_GAIN;

			m_pitch -= delta.y;
			m_yaw -= delta.x;

			// limit pitch to straight up or straight down
			// with a little fudge-factor to avoid gimbal lock
			float limit = XM_PI / 2.0f - 0.01f;
			m_pitch = max(-limit, m_pitch);
			m_pitch = min(+limit, m_pitch);

			// keep longitude in sane range by wrapping
			if (m_yaw > XM_PI)
			{
				m_yaw -= XM_PI * 2.0f;
			}
			else if (m_yaw < -XM_PI)
			{
				m_yaw += XM_PI * 2.0f;
			}
		}
		XMMATRIX view;
		XMMATRIX invView;
		_mouse->SetMode(mouse.leftButton ? Mouse::MODE_RELATIVE : Mouse::MODE_ABSOLUTE);
		{
			float y = sinf(m_pitch);
			float r = -cosf(m_pitch);
			float x = r*cosf(m_yaw);
			float z = r*sinf(m_yaw);
			XMVECTOR lookAt = XMLoadFloat3(&m_cameraPos) + XMLoadFloat3(&XMFLOAT3(x, y, z));
			auto Up = XMLoadFloat3(&XMFLOAT3(0, 1, 0));

			view = XMMatrixLookAtLH(XMLoadFloat3(&m_cameraPos), lookAt, Up);
			invView = DirectX::XMMatrixInverse(0, view);

			DirectX::XMStoreFloat4x4(&scene_state.mView, DirectX::XMMatrixTranspose(view));
			DirectX::XMStoreFloat4x4(&scene_state.mInvView, DirectX::XMMatrixTranspose(invView));
		}
		auto kb = _keyboard->GetState();
		{
			SimpleMath::Vector3 move = SimpleMath::Vector3::Zero;

			if (kb.F1)
				fire = true;

			if (kb.F2)
			{
				velocity = SimpleMath::Vector3(0, 0, 0);
				fire = false;
				globalCollision = false;
			}

			if (kb.F3)
			{
				forcePush = true;
			}

			if (kb.F4)
			{
				forcePush = false;
			}

			if (kb.Up || kb.W)
				move.z += 1.f;

			if (kb.Down || kb.S)
				move.z -= 1.f;

			if (kb.Left || kb.A)
				move.x -= 1.f;

			if (kb.Right || kb.D)
				move.x += 1.f;

			if (kb.PageUp || kb.Space)
				move.y += 1.f;

			if (kb.PageDown || kb.X)
				move.y -= 1.f;

			move = DirectX::XMVector3TransformNormal(XMLoadFloat3(&move), invView);

			//SimpleMath::Quaternion q = SimpleMath::Quaternion::CreateFromYawPitchRoll(0, m_yaw, m_pitch); //m_yaw, m_pitch

			//move = SimpleMath::Vector3::Transform(move, q);

			move *= MOVEMENT_GAIN;

			auto cp = XMLoadFloat3(&m_cameraPos);
			cp += move;

			XMStoreFloat3(&m_cameraPos, cp);
		}
	}
}