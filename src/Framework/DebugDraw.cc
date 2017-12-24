#include "DebugDraw.h"
#include "imgui.h"
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "shaders.h"

DebugDraw g_debugDraw;
Camera g_camera;

using namespace Oryol;

struct Color {
	union {
		struct {
			uint8_t r, g, b, a;
		};
		uint32_t value;
	};
	inline Color(float _r, float _g, float _b, float _a = 1.0f)
		: r(_r * 255), g(_g * 255), b(_b * 255), a(_a * 255) {}
	inline Color(const b2Color & color)
		: Color(color.r,color.g,color.b,color.a) {}
};

glm::mat3x2 BuildAffineTransform2D(const glm::vec2 & origin, const float rotation, const glm::vec2 & scale, const glm::vec2 & position)
{
	float rot_radians = glm::radians(rotation);
	float cos = std::cos(rot_radians);
	float sin = std::sin(rot_radians);
	auto scaled_origin = glm::vec2(origin.x * scale.x, origin.y * scale.y);
	auto transformed_origin = glm::vec2(
		scaled_origin.x * cos - scaled_origin.y * sin,
		scaled_origin.x * sin + scaled_origin.y * cos
	);
	return glm::mat3x2(
		scale.x * cos, scale.x * sin,
		scale.y * -sin, scale.y * cos,
		position.x - transformed_origin.x, position.y - transformed_origin.y
	);
}

glm::mat3x3 AffineTransform2DToMat3(const glm::mat3x2 & m)
{
	return glm::mat3x3(
		m[0][0], m[0][1], 0,
		m[1][0], m[1][1], 0,
		m[2][0], m[2][1], 1
	);
}
CameraSetup::CameraSetup()
	: WorldPosition(0, 0), Rotation(0), Zoom(1), zNear(-10), zFar(10)
{
	auto & display = Oryol::Gfx::DisplayAttrs();
	ViewportHeight = display.FramebufferHeight;
	ViewportWidth = display.FramebufferWidth;
}
CameraSetup::CameraSetup(uint32_t viewportWidth, uint32_t viewportHeight)
	: WorldPosition(0, 0), Rotation(0), Zoom(1), ViewportHeight(viewportHeight), ViewportWidth(viewportWidth), zNear(-10), zFar(10)
{
}
void Camera::Setup(const CameraSetup & setup)
{
	WorldPosition = setup.WorldPosition;
	Zoom = setup.Zoom;
	Rotation = setup.Rotation;
	zFar = setup.zFar;
	zNear = setup.zNear;
	ResizeViewport(setup.ViewportWidth, setup.ViewportHeight);
	Update();
}

void Camera::Update()
{
	auto xform = BuildAffineTransform2D(this->WorldPosition, this->Rotation, { this->Zoom, this->Zoom }, this->viewportCenter);
	this->worldToScreenMatrix = glm::mat2(xform);
	this->screenToWorldMatrix = glm::inverse(this->worldToScreenMatrix);
	this->translationOffset = xform[2];
}

glm::vec2 Camera::ConvertScreenToWorld(const glm::vec2 & screenPoint)
{
	return this->screenToWorldMatrix*(screenPoint - this->translationOffset);
}

glm::vec2 Camera::ConvertWorldToScreen(const glm::vec2 & worldPoint)
{
	return this->worldToScreenMatrix * worldPoint + this->translationOffset;

}

glm::vec2 Camera::ConvertScreenToWorldDir(const glm::vec2 & screenDir)
{
	return this->screenToWorldMatrix * screenDir;
}

glm::vec2 Camera::ConvertWorldToScreenDir(const glm::vec2 & worldDir)
{
	return this->worldToScreenMatrix * worldDir;
}

glm::mat4 Camera::BuildProjectionViewMatrix(float zBias)
{
	return projectionMatrix * glm::mat4x4(
		this->worldToScreenMatrix[0][0], this->worldToScreenMatrix[0][1], 0, 0,
		this->worldToScreenMatrix[1][0], this->worldToScreenMatrix[1][1], 0, 0,
		0, 0, 1, 0,
		this->translationOffset.x, this->translationOffset.y, zBias, 1
	);
}

void Camera::ResizeViewport(uint32_t width, uint32_t height) {
	viewportCenter = { width * 0.5f, height * 0.5f };
	projectionMatrix = glm::ortho<float>(0, width, 0, height, zNear, zFar);
}

float Camera::GetWidth() const { return viewportCenter.x * 2; }

float Camera::GetHeight() const { return viewportCenter.y * 2; }

void DebugDraw::DrawPolygon(const b2Vec2 * vertices, int32 vertexCount, const b2Color & color)
{
	b2Vec2 p1 = vertices[vertexCount - 1];
	for (int32 i = 0; i < vertexCount; ++i)
	{
		b2Vec2 p2 = vertices[i];
		LineVertex(p1, color);
		LineVertex(p2, color);
		p1 = p2;
	}
}

void DebugDraw::DrawSolidPolygon(const b2Vec2 * vertices, int32 vertexCount, const b2Color & color)
{
	b2Color fillColor(0.5f * color.r, 0.5f * color.g, 0.5f * color.b, 0.5f);

	for (int32 i = 1; i < vertexCount - 1; ++i)
	{
		TriangleVertex(vertices[0], fillColor);
		TriangleVertex(vertices[i], fillColor);
		TriangleVertex(vertices[i + 1], fillColor);
	}

	b2Vec2 p1 = vertices[vertexCount - 1];
	for (int32 i = 0; i < vertexCount; ++i)
	{
		b2Vec2 p2 = vertices[i];
		LineVertex(p1, color);
		LineVertex(p2, color);
		p1 = p2;
	}
}
void DebugDraw::DrawCircle(const b2Vec2& center, float32 radius, const b2Color& color)
{
	const float32 k_segments = 16.0f;
	const float32 k_increment = 2.0f * b2_pi / k_segments;
	float32 sinInc = sinf(k_increment);
	float32 cosInc = cosf(k_increment);
	b2Vec2 r1(1.0f, 0.0f);
	b2Vec2 v1 = center + radius * r1;
	for (int32 i = 0; i < k_segments; ++i)
	{
		// Perform rotation to avoid additional trigonometry.
		b2Vec2 r2;
		r2.x = cosInc * r1.x - sinInc * r1.y;
		r2.y = sinInc * r1.x + cosInc * r1.y;
		b2Vec2 v2 = center + radius * r2;
		LineVertex(v1, color);
		LineVertex(v2, color);
		r1 = r2;
		v1 = v2;
	}
}

void DebugDraw::DrawSolidCircle(const b2Vec2& center, float32 radius, const b2Vec2& axis, const b2Color& color)
{

	const float32 k_segments = 16.0f;
	const float32 k_increment = 2.0f * b2_pi / k_segments;
	float32 sinInc = sinf(k_increment);
	float32 cosInc = cosf(k_increment);
	b2Vec2 v0 = center;
	b2Vec2 r1(cosInc, sinInc);
	b2Vec2 v1 = center + radius * r1;
	b2Color fillColor(0.5f * color.r, 0.5f * color.g, 0.5f * color.b, 0.5f);
	for (int32 i = 0; i < k_segments; ++i)
	{
		// Perform rotation to avoid additional trigonometry.
		b2Vec2 r2;
		r2.x = cosInc * r1.x - sinInc * r1.y;
		r2.y = sinInc * r1.x + cosInc * r1.y;
		b2Vec2 v2 = center + radius * r2;
		TriangleVertex(v0, fillColor);
		TriangleVertex(v1, fillColor);
		TriangleVertex(v2, fillColor);
		r1 = r2;
		v1 = v2;
	}

	r1.Set(1.0f, 0.0f);
	v1 = center + radius * r1;
	for (int32 i = 0; i < k_segments; ++i)
	{
		b2Vec2 r2;
		r2.x = cosInc * r1.x - sinInc * r1.y;
		r2.y = sinInc * r1.x + cosInc * r1.y;
		b2Vec2 v2 = center + radius * r2;
		LineVertex(v1, color);
		LineVertex(v2, color);
		r1 = r2;
		v1 = v2;
	}

	// Draw a line fixed in the circle to animate rotation.
	b2Vec2 p = center + radius * axis;
	LineVertex(center, color);
	LineVertex(p, color);
}

void DebugDraw::DrawSegment(const b2Vec2& p1, const b2Vec2& p2, const b2Color& color)
{
	LineVertex(p1, color);
	LineVertex(p2, color);
}

//
void DebugDraw::DrawTransform(const b2Transform& xf)
{
	const float32 k_axisScale = 0.4f;
	b2Color red(1.0f, 0.0f, 0.0f);
	b2Color green(0.0f, 1.0f, 0.0f);
	b2Vec2 p1 = xf.p, p2;

	LineVertex(p1, red);
	p2 = p1 + k_axisScale * xf.q.GetXAxis();
	LineVertex(p2, red);

	LineVertex(p1, green);
	p2 = p1 + k_axisScale * xf.q.GetYAxis();
	LineVertex(p2, green);
}

//
void DebugDraw::DrawPoint(const b2Vec2& p, float32 size, const b2Color& color)
{
	PointVertex(p, color, size);
}

//
void DebugDraw::DrawString(int x, int y, const char *string, ...)
{
	va_list arg;
	va_start(arg, string);
	ImGui::Begin("Overlay", NULL, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar);
	ImGui::SetCursorPos(ImVec2(float(x), float(y)));
	ImGui::TextColoredV(ImColor(230, 153, 153, 255), string, arg);
	ImGui::End();
	va_end(arg);
}

//
void DebugDraw::DrawString(const b2Vec2& pw, const char *string, ...)
{
	auto ps = g_camera.ConvertWorldToScreen({ pw.x,pw.y });

	va_list arg;
	va_start(arg, string);
	ImGui::Begin("Overlay", NULL, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar);
	ImGui::SetCursorPos(ImVec2(ps.x,ps.y));
	ImGui::TextColoredV(ImColor(230, 153, 153, 255), string, arg);
	ImGui::End();
	va_end(arg);
}

//
void DebugDraw::DrawAABB(b2AABB* aabb, const b2Color& c)
{
	b2Vec2 p1 = aabb->lowerBound;
	b2Vec2 p2 = b2Vec2(aabb->upperBound.x, aabb->lowerBound.y);
	b2Vec2 p3 = aabb->upperBound;
	b2Vec2 p4 = b2Vec2(aabb->lowerBound.x, aabb->upperBound.y);

	LineVertex(p1, c);
	LineVertex(p2, c);

	LineVertex(p2, c);
	LineVertex(p3, c);

	LineVertex(p3, c);
	LineVertex(p4, c);

	LineVertex(p4, c);
	LineVertex(p1, c);
}

void DebugDraw::Setup(const Oryol::GfxSetup & setup)
{
	Gfx::PushResourceLabel();
	{
		//Setup triangle draw state
		auto meshSetup = MeshSetup::Empty(MaxNumTriangleVertices, Usage::Stream);
		meshSetup.Layout = {
			{ VertexAttr::Position, VertexFormat::Float2 },
			{ VertexAttr::Color0, VertexFormat::UByte4N }
		};
		this->drawState[0].Mesh[0] = Gfx::CreateResource(meshSetup);
		//Setup up the shader and pipeline for rendering
		Id shd = Gfx::CreateResource(DebugGeometryShader::Setup());
		auto pipSetup = PipelineSetup::FromLayoutAndShader(meshSetup.Layout, shd);
		pipSetup.RasterizerState.SampleCount = setup.SampleCount;
		pipSetup.BlendState.ColorFormat = setup.ColorFormat;
		pipSetup.BlendState.DepthFormat = setup.DepthFormat;
		pipSetup.PrimType = PrimitiveType::Triangles;
		//Setup Blending
		pipSetup.BlendState.BlendEnabled = true;
		pipSetup.BlendState.SrcFactorRGB = BlendFactor::SrcAlpha;
		pipSetup.BlendState.DstFactorRGB = BlendFactor::OneMinusSrcAlpha;
		this->drawState[0].Pipeline = Gfx::CreateResource(pipSetup);
	}
	{
		//Setup line draw state
		auto meshSetup = MeshSetup::Empty(MaxNumLineVertices, Usage::Stream);
		meshSetup.Layout = {
			{ VertexAttr::Position, VertexFormat::Float2 },
			{ VertexAttr::Color0, VertexFormat::UByte4N }
		};
		this->drawState[1].Mesh[0] = Gfx::CreateResource(meshSetup);
		//Setup up the shader and pipeline for rendering
		Id shd = Gfx::CreateResource(DebugGeometryShader::Setup());
		auto pipSetup = PipelineSetup::FromLayoutAndShader(meshSetup.Layout, shd);
		pipSetup.RasterizerState.SampleCount = setup.SampleCount;
		pipSetup.BlendState.ColorFormat = setup.ColorFormat;
		pipSetup.BlendState.DepthFormat = setup.DepthFormat;
		pipSetup.PrimType = PrimitiveType::Lines;
		//Setup Blending
		pipSetup.BlendState.BlendEnabled = true;
		pipSetup.BlendState.SrcFactorRGB = BlendFactor::SrcAlpha;
		pipSetup.BlendState.DstFactorRGB = BlendFactor::OneMinusSrcAlpha;
		this->drawState[1].Pipeline = Gfx::CreateResource(pipSetup);
	}
	{
		//Setup point draw state
		static struct data_t {
			const float vertices[4][2] = { { -0.5f,-0.5f },{ 0.5f,-0.5f },{ -0.5f,0.5f },{ 0.5f,0.5f } };
			const uint16_t indices[6] = { 0,1,2,1,3,2 };
		} data;
		auto pointMeshSetup = MeshSetup::FromData();
		pointMeshSetup.NumVertices = 4;
		pointMeshSetup.NumIndices = 6;
		pointMeshSetup.IndicesType = IndexType::Index16;
		pointMeshSetup.Layout = {
			{ VertexAttr::TexCoord0, VertexFormat::Float2 }
		};
		pointMeshSetup.AddPrimitiveGroup({ 0, 6 });
		pointMeshSetup.VertexDataOffset = 0;
		pointMeshSetup.IndexDataOffset = offsetof(data_t, indices);
		this->drawState[2].Mesh[0] = Gfx::CreateResource(pointMeshSetup, &data, sizeof(data));

		auto instanceMeshSetup = MeshSetup::Empty(MaxNumPointVertices, Usage::Stream);
		instanceMeshSetup.Layout
			.EnableInstancing()
			.Add(VertexAttr::Instance0, VertexFormat::Float3) //x, y, size
			.Add(VertexAttr::Instance1, VertexFormat::UByte4N); //Color
		instanceMeshSetup.Layout.EnableInstancing();
		this->drawState[2].Mesh[1] = Gfx::CreateResource(instanceMeshSetup);
		Id shd = Gfx::CreateResource(DebugPointShader::Setup());
		auto pipSetup = PipelineSetup::FromShader(shd);
		pipSetup.Layouts[0] = pointMeshSetup.Layout;
		pipSetup.Layouts[1] = instanceMeshSetup.Layout;
		pipSetup.RasterizerState.SampleCount = setup.SampleCount;
		pipSetup.BlendState.ColorFormat = setup.ColorFormat;
		pipSetup.BlendState.DepthFormat = setup.DepthFormat;
		pipSetup.PrimType = PrimitiveType::Triangles;
		//Setup Blending
		pipSetup.BlendState.BlendEnabled = true;
		pipSetup.BlendState.SrcFactorRGB = BlendFactor::SrcAlpha;
		pipSetup.BlendState.DstFactorRGB = BlendFactor::OneMinusSrcAlpha;
		this->drawState[2].Pipeline = Gfx::CreateResource(pipSetup);
	}
	this->label = Gfx::PopResourceLabel();
}

void DebugDraw::Discard()
{
	Gfx::DestroyResources(this->label);
	this->label.Invalidate();
}

void DebugDraw::Render(const glm::mat4 & mvpMatrix)
{
	DebugGeometryShader::vsParams params{ mvpMatrix };
	if (!triangles.Empty()) {
		Gfx::UpdateVertices(this->drawState[0].Mesh[0], this->triangles.begin(), this->triangles.Size() * sizeof(vertex_t));
		Gfx::ApplyDrawState(this->drawState[0]);
		Gfx::ApplyUniformBlock(params);
		Gfx::Draw({ 0,this->triangles.Size() });
		triangles.Clear();
	}
	if (!lines.Empty()) {
		Gfx::UpdateVertices(this->drawState[1].Mesh[0], this->lines.begin(), this->lines.Size() * sizeof(vertex_t));
		Gfx::ApplyDrawState(this->drawState[1]);
		Gfx::ApplyUniformBlock(params);
		Gfx::Draw({ 0,this->lines.Size() });
		lines.Clear();
	}
	if (!points.Empty()) {
		Gfx::UpdateVertices(this->drawState[2].Mesh[1], this->points.begin(), this->points.Size() * sizeof(instance_t));
		Gfx::ApplyDrawState(this->drawState[2]);
		Gfx::ApplyUniformBlock(params);
		Gfx::Draw(0, this->points.Size());
		this->points.Clear();
	}
}

void DebugDraw::LineVertex(const b2Vec2 & position, const b2Color & color)
{
	if (lines.Size() < MaxNumLineVertices)
		lines.Add({ position.x,position.y,Color(color).value });
}

void DebugDraw::TriangleVertex(const b2Vec2 & position, const b2Color & color)
{
	if (triangles.Size() < MaxNumTriangleVertices)
		triangles.Add({ position.x,position.y,Color(color).value });
}

void DebugDraw::PointVertex(const b2Vec2 & position, const b2Color & color, float32 size)
{
	if (points.Size() < MaxNumPointVertices)
		points.Add({ position.x,position.y,size/g_camera.Zoom,Color(color).value });
}
