#include "libs/glad_loader_docs.h"
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <iostream>
#include <cstdlib>
#include <ctime>
#include <cmath>
#include "PolygonWithTexture.cpp"
#include "Polygon.cpp"
#include <learnopengl/shader_m.h>
#include <learnopengl/camera.h>
#include "stb_image.h"

using namespace glm;

int SCR_WIDTH = 1280;
int SCR_HEIGHT = 720;

// الكاميرا الابتدائية: الحالة 1 (من فوق الطاولة تماماً)
Camera camera(vec3(0.0f, 3.6f, 0.25f), vec3(0.0f, 1.0f, 0.0f), -90.0f, -86.0f);
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;
bool freeCameraMode = false;
bool rPressed = false;
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// متغيرات الكاميرا والانتقال السلس بين الحالتين (1 و 2)
int cameraState = 1;
vec3 camPos1 = vec3(0.0f, 3.6f, 0.25f);
float yaw1 = -90.0f, pitch1 = -86.0f;

vec3 camPos2 = vec3(0.0f, 2.05f, 2.45f);
float yaw2 = -90.0f, pitch2 = -34.0f;

vec3 targetCamPos = camPos1;
float targetYaw = yaw1;
float targetPitch = pitch1;

// زاوية وقوف بطاقات المخزون: 0 = أفقية (حالة 1)، 68 = واقفة طولياً (حالة 2)
float currentStandAngle = 0.0f;
float targetStandAngle = 0.0f;

struct CardItem
{
	PolygonWithTexture *mesh;
	vec3 deckPos;
	vec3 playedPos;
	vec3 currentPos;
	float moveProgress;
	bool isPlayed;
};

std::vector<CardItem> cards;
bool gKeyWasPressed = false;
bool cKeyWasPressed = false;

unsigned int loadTexture(const char *path)
{
	unsigned int textureID;
	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_2D, textureID);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	int width, height, nrChannels;
	unsigned char *data = stbi_load(path, &width, &height, &nrChannels, 0);
	if (data)
	{
		GLenum format = (nrChannels == 4) ? GL_RGBA : GL_RGB;
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);
	}
	else
	{
		std::cout << "Failed to load texture: " << path << std::endl;
	}
	stbi_image_free(data);
	return textureID;
}

PolygonWithTexture *createTexturedQuad(vec3 p0, vec3 p1, vec3 p2, vec3 p3, unsigned int tex, float repeatX = 1.0f, float repeatY = 1.0f, vec3 color = vec3(1.0f))
{
	std::vector<vec3> verts = {p0, p1, p2, p3};
	std::vector<vec2> uvs = {
		vec2(0.0f, 0.0f),
		vec2(repeatX, 0.0f),
		vec2(repeatX, repeatY),
		vec2(0.0f, repeatY)};
	PolygonWithTexture *quad = new PolygonWithTexture(verts, uvs, color);
	quad->setTexture(tex);
	return quad;
}

std::vector<PolygonWithTexture *> createTexturedBox(float w, float h, float d, unsigned int texFront, unsigned int texSide, vec3 tint = vec3(1.0f))
{
	std::vector<PolygonWithTexture *> faces;
	float hw = w / 2.0f, hh = h / 2.0f, hd = d / 2.0f;

	faces.push_back(createTexturedQuad(vec3(-hw, -hh, hd), vec3(hw, -hh, hd), vec3(hw, hh, hd), vec3(-hw, hh, hd), texFront, 1.0f, 1.0f, vec3(1.0f)));
	faces.push_back(createTexturedQuad(vec3(hw, -hh, -hd), vec3(-hw, -hh, -hd), vec3(-hw, hh, -hd), vec3(hw, hh, -hd), texSide, 1.0f, 1.0f, tint));
	faces.push_back(createTexturedQuad(vec3(hw, -hh, hd), vec3(hw, -hh, -hd), vec3(hw, hh, -hd), vec3(hw, hh, hd), texSide, 1.0f, 1.0f, tint));
	faces.push_back(createTexturedQuad(vec3(-hw, -hh, -hd), vec3(-hw, -hh, hd), vec3(-hw, hh, hd), vec3(-hw, hh, -hd), texSide, 1.0f, 1.0f, tint));
	faces.push_back(createTexturedQuad(vec3(-hw, hh, hd), vec3(hw, hh, hd), vec3(hw, hh, -hd), vec3(-hw, hh, -hd), texSide, 1.0f, 1.0f, tint));
	faces.push_back(createTexturedQuad(vec3(-hw, -hh, -hd), vec3(hw, -hh, -hd), vec3(hw, -hh, hd), vec3(-hw, -hh, hd), texSide, 1.0f, 1.0f, tint));

	return faces;
}

void processInput(GLFWwindow *window);
void mouse_callback(GLFWwindow *window, double xposIn, double yposIn);
void framebuffer_size_callback(GLFWwindow *window, int width, int height);

int main()
{
	srand(static_cast<unsigned int>(time(0)));

	glfwInit();
	GLFWwindow *window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Conan Card Game - OpenGL Project", NULL, NULL);
	glfwMakeContextCurrent(window);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	glfwSetCursorPosCallback(window, mouse_callback);

	gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
	glEnable(GL_DEPTH_TEST);
	stbi_set_flip_vertically_on_load(true);

	Shader ourShader("./shaders/vs/L4.vs", "./shaders/fs/L4.fs");

	// 1. تحميل الصور
	unsigned int texCards[5];
	texCards[0] = loadTexture("./textures/card1.jpg");
	texCards[1] = loadTexture("./textures/card2.jpg");
	texCards[2] = loadTexture("./textures/card3.jpg");
	texCards[3] = loadTexture("./textures/card4.jpg");
	texCards[4] = loadTexture("./textures/card5.jpg");

	unsigned int texFloor = loadTexture("./textures/floor_wood.jpg");

	unsigned int texTable = loadTexture("./textures/floor_wood.jpg");
	unsigned int texTableMat = loadTexture("./textures/floor_wood.jpg");
	unsigned int texDecor1 = loadTexture("./textures/decor1_3d.jpg");
	unsigned int texDecor2 = loadTexture("./textures/decor2_2d.jpg");
	unsigned int texDecor3 = loadTexture("./textures/decor3_2d.jpg");
	unsigned int texPoster = loadTexture("./textures/poster.jpg");
	unsigned int texCeilingOrnate = loadTexture("textures/ceiling_ornate.jpg");
	unsigned int texWallWindows = loadTexture("textures/wall_windows.jpg");
	unsigned int texWallDoor = loadTexture("textures/wall_door.jpg");
	unsigned int texWallPlain = loadTexture("textures/wall_plain.jpg");
	// 2. بناء الغرفة
	float roomW = 6.5f, roomH = 5.0f, roomD = 6.5f;
	PolygonWithTexture *floorQuad = createTexturedQuad(
		vec3(-roomW, 0.0f, roomD), vec3(roomW, 0.0f, roomD),
		vec3(roomW, 0.0f, -roomD), vec3(-roomW, 0.0f, -roomD),
		texFloor, 4.0f, 4.0f);
	// الجدار الخلفي (يأخذ الصورة العاجية المزخرفة كاملة مع الديكور السفلي، والبوستر يبقى أمامه بوضوح)
	PolygonWithTexture *backWall = createTexturedQuad(
		vec3(-roomW, 0.0f, -roomD), vec3(roomW, 0.0f, -roomD),
		vec3(roomW, roomH, -roomD), vec3(-roomW, roomH, -roomD), texWallPlain, 1.0f, 1.0f);

	// الجدار اليساري المقابل للشباك (يأخذ نفس الصورة العاجية المزخرفة كاملة)
	PolygonWithTexture *leftWall = createTexturedQuad(
		vec3(-roomW, 0.0f, roomD), vec3(-roomW, 0.0f, -roomD),
		vec3(-roomW, roomH, -roomD), vec3(-roomW, roomH, roomD), texWallPlain, 1.0f, 1.0f);
	// الجدار الأمامي (يأخذ صورة جدار الباب)
	PolygonWithTexture *frontWall = createTexturedQuad(
		vec3(roomW, 0.0f, roomD), vec3(-roomW, 0.0f, roomD),
		vec3(-roomW, roomH, roomD), vec3(roomW, roomH, roomD), texWallDoor);

	// الجدار اليميني (يأخذ صورة جدار الشبابيك والستائر)
	PolygonWithTexture *rightWall = createTexturedQuad(
		vec3(roomW, 0.0f, -roomD), vec3(roomW, 0.0f, roomD),
		vec3(roomW, roomH, roomD), vec3(roomW, roomH, -roomD), texWallWindows);

	// السقف (يأخذ صورة السقف الجبسي المزخرف)
	PolygonWithTexture *ceilingQuad = createTexturedQuad(
		vec3(-roomW, roomH, -roomD), vec3(roomW, roomH, -roomD),
		vec3(roomW, roomH, roomD), vec3(-roomW, roomH, roomD), texCeilingOrnate);

	// 3. البوستر الممزق من الزاوية السفلية اليمنى
	std::vector<vec3> posterVerts = {
		vec3(-2.6f, 1.3f, -roomD + 0.02f),
		vec3(-1.35f, 1.3f, -roomD + 0.02f),
		vec3(-1.55f, 1.55f, -roomD + 0.02f),
		vec3(-1.25f, 1.75f, -roomD + 0.02f),
		vec3(-1.42f, 2.05f, -roomD + 0.02f),
		vec3(-1.1f, 2.25f, -roomD + 0.02f),
		vec3(-1.1f, 3.9f, -roomD + 0.02f),
		vec3(-2.6f, 3.9f, -roomD + 0.02f)};
	std::vector<vec2> posterUVs = {
		vec2(0.0f, 0.0f),
		vec2(0.83f, 0.0f),
		vec2(0.70f, 0.10f),
		vec2(0.90f, 0.17f),
		vec2(0.78f, 0.29f),
		vec2(1.0f, 0.36f),
		vec2(1.0f, 1.0f),
		vec2(0.0f, 1.0f)};
	PolygonWithTexture *tornPoster = new PolygonWithTexture(posterVerts, posterUVs);
	tornPoster->setTexture(texPoster);

	// 4. الطاولة وأقسامها الثلاثة
	float tableTopY = 1.0f;
	std::vector<PolygonWithTexture *> tableTop = createTexturedBox(3.4f, 0.12f, 2.2f, texTable, texTable, vec3(0.75f, 0.58f, 0.45f));
	for (auto *f : tableTop)
		f->transformation(translate(mat4(1.0f), vec3(0.0f, tableTopY - 0.06f, 0.0f)));

	vec3 legOffsets[4] = {
		vec3(-1.5f, 0.47f, -0.9f), vec3(1.5f, 0.47f, -0.9f),
		vec3(-1.5f, 0.47f, 0.9f), vec3(1.5f, 0.47f, 0.9f)};
	std::vector<std::vector<PolygonWithTexture *>> tableLegs;
	for (int i = 0; i < 4; i++)
	{
		auto leg = createTexturedBox(0.14f, 0.94f, 0.14f, texTable, texTable, vec3(0.55f, 0.40f, 0.30f));
		for (auto *f : leg)
			f->transformation(translate(mat4(1.0f), legOffsets[i]));
		tableLegs.push_back(leg);
	}

	PolygonWithTexture *secOpponent = createTexturedQuad(
		vec3(-1.35f, tableTopY + 0.003f, -0.38f), vec3(1.35f, tableTopY + 0.003f, -0.38f),
		vec3(1.35f, tableTopY + 0.003f, -0.92f), vec3(-1.35f, tableTopY + 0.003f, -0.92f),
		texTableMat, 2.0f, 1.0f, vec3(0.55f, 0.35f, 0.35f));
	PolygonWithTexture *secPlayed = createTexturedQuad(
		vec3(-1.35f, tableTopY + 0.003f, 0.27f), vec3(1.35f, tableTopY + 0.003f, 0.27f),
		vec3(1.35f, tableTopY + 0.003f, -0.27f), vec3(-1.35f, tableTopY + 0.003f, -0.27f),
		texTableMat, 2.0f, 1.0f, vec3(0.35f, 0.55f, 0.45f));
	PolygonWithTexture *secDeck = createTexturedQuad(
		vec3(-1.35f, tableTopY + 0.003f, 0.92f), vec3(1.35f, tableTopY + 0.003f, 0.92f),
		vec3(1.35f, tableTopY + 0.003f, 0.38f), vec3(-1.35f, tableTopY + 0.003f, 0.38f),
		texTableMat, 2.0f, 1.0f, vec3(0.35f, 0.45f, 0.65f));

	// 5. إنشاء بطاقات اللاعب الخمسة في قسم المخزون (Deck) + كومة ورق مكدسة على يمين الطاولة
	float cardW = 0.34f;
	float cardH = 0.48f;
	for (int i = 0; i < 5; i++)
	{
		float xPos = -0.85f + i * 0.42f;
		PolygonWithTexture *cMesh = createTexturedQuad(
			vec3(-cardW / 2.0f, 0.0f, 0.0f),
			vec3(cardW / 2.0f, 0.0f, 0.0f),
			vec3(cardW / 2.0f, 0.0f, -cardH),
			vec3(-cardW / 2.0f, 0.0f, -cardH),
			texCards[i]);

		CardItem item;
		item.mesh = cMesh;
		item.deckPos = vec3(xPos, tableTopY + 0.01f + i * 0.001f, 0.88f);
		item.playedPos = vec3(xPos, tableTopY + 0.01f, 0.23f);
		item.currentPos = item.deckPos;
		item.moveProgress = 0.0f;
		item.isPlayed = false;
		cards.push_back(item);
	}

	// كومة أوراق مكدسة على جنب الطاولة (يمين قسم اللعب) والبطاقة العلوية عليها صورة كونان
	std::vector<PolygonWithTexture *> sideCardStack;
	for (int i = 0; i < 6; i++)
	{
		float yStack = tableTopY + 0.006f + i * 0.008f;
		unsigned int cardTex = (i == 5) ? texCards[0] : texTableMat;
		vec3 cardTint = (i == 5) ? vec3(1.0f) : vec3(0.25f, 0.32f, 0.50f);

		PolygonWithTexture *stackCard = createTexturedQuad(
			vec3(1.12f, yStack, 0.22f),
			vec3(1.44f, yStack, 0.22f),
			vec3(1.44f, yStack, -0.22f),
			vec3(1.12f, yStack, -0.22f),
			cardTex, 1.0f, 1.0f, cardTint);
		sideCardStack.push_back(stackCard);
	}
	// 6. طاولة ديكور في زاوية الغرفة + القطع الثلاث متناسقة ومتقاربة بنعومة
	float sideTableY = 0.95f;
	float stW = 3.50f, stD = 1.40f;
	float stCenterX = -4.40f, stCenterZ = -4.85f;

	auto sideTableTop = createTexturedBox(stW, 0.10f, stD, texTable, texTable, vec3(0.45f, 0.30f, 0.20f));
	for (auto *f : sideTableTop)
		f->transformation(translate(mat4(1.0f), vec3(stCenterX, sideTableY - 0.05f, stCenterZ)));

	std::vector<PolygonWithTexture *> sideTableLegs;
	float sLegX[2] = {stCenterX - stW / 2.0f + 0.12f, stCenterX + stW / 2.0f - 0.12f};
	float sLegZ[2] = {stCenterZ - stD / 2.0f + 0.12f, stCenterZ + stD / 2.0f - 0.12f};
	for (int i = 0; i < 2; i++)
	{
		for (int j = 0; j < 2; j++)
		{
			auto leg = createTexturedBox(0.11f, sideTableY - 0.10f, 0.11f, texTable, texTable, vec3(0.32f, 0.20f, 0.13f));
			for (auto *f : leg)
				f->transformation(translate(mat4(1.0f), vec3(sLegX[i], (sideTableY - 0.10f) / 2.0f, sLegZ[j])));
			sideTableLegs.insert(sideTableLegs.end(), leg.begin(), leg.end());
		}
	}

	// القطعة 1: مجسم 3D صلب (فانوس كونان ثلاثي الأبعاد بـ 6 وجوه + صورة كونان بداخل الواجهة)
	std::vector<PolygonWithTexture *> lantern3D;
	vec3 woodDark = vec3(0.32f, 0.20f, 0.12f);
	vec3 woodTrim = vec3(0.55f, 0.36f, 0.20f);
	vec3 goldTrim = vec3(0.85f, 0.68f, 0.30f);

	float posX = stCenterX - 1.25f; // أقصى يسار الطاولة بتناسق
	float posZ = stCenterZ + 0.05f;
	float rotY = radians(22.0f);

	auto addSolidPart = [&](float w, float h, float d, vec3 localPos, unsigned int tex, vec3 col)
	{
		auto box = createTexturedBox(w, h, d, tex, tex, col);
		mat4 m = translate(mat4(1.0f), vec3(posX, sideTableY, posZ));
		m = rotate(m, rotY, vec3(0.0f, 1.0f, 0.0f));
		m = translate(m, localPos);
		for (auto *f : box)
			f->transformation(m);
		lantern3D.insert(lantern3D.end(), box.begin(), box.end());
	};

	// 1. القاعدة السفلية المدرّجة
	addSolidPart(0.64f, 0.08f, 0.56f, vec3(0.0f, 0.04f, 0.0f), texTable, woodDark);
	addSolidPart(0.56f, 0.14f, 0.48f, vec3(0.0f, 0.15f, 0.0f), texTable, woodTrim);
	addSolidPart(0.60f, 0.04f, 0.52f, vec3(0.0f, 0.24f, 0.0f), texTable, goldTrim);

	// 2. البدن الخشبي الصلب + حشوات صورة كونان (texDecor1) على الوجوه الأربعة بدون أي سواد!
	addSolidPart(0.48f, 0.68f, 0.42f, vec3(0.0f, 0.60f, 0.0f), texTable, woodTrim);
	addSolidPart(0.34f, 0.50f, 0.44f, vec3(0.0f, 0.60f, 0.0f), texDecor1, vec3(1.0f, 1.0f, 1.0f));
	addSolidPart(0.50f, 0.50f, 0.30f, vec3(0.0f, 0.60f, 0.0f), texDecor1, vec3(1.0f, 1.0f, 1.0f));

	// 3. أربعة أعمدة زاوية بارزة 3D
	float cx[2] = {-0.23f, 0.23f};
	float cz[2] = {-0.20f, 0.20f};
	for (int i = 0; i < 2; i++)
		for (int j = 0; j < 2; j++)
			addSolidPart(0.06f, 0.70f, 0.06f, vec3(cx[i], 0.60f, cz[j]), texTable, woodDark);

	// 4. التاج العلوي المدرّج والقبة الذهبية
	addSolidPart(0.60f, 0.06f, 0.52f, vec3(0.0f, 0.97f, 0.0f), texTable, woodDark);
	addSolidPart(0.46f, 0.07f, 0.40f, vec3(0.0f, 1.035f, 0.0f), texTable, goldTrim);
	addSolidPart(0.26f, 0.06f, 0.24f, vec3(0.0f, 1.10f, 0.0f), texTable, woodDark);
	addSolidPart(0.08f, 0.10f, 0.08f, vec3(0.0f, 1.18f, 0.0f), texTable, goldTrim);

	// القطعة 2: مجسم قوس كونان في منتصف الطاولة (حجم أكبر ومسافة متناسقة)
	PolygonWithTexture *decor3_2D = createTexturedQuad(
		vec3(-0.44f, 0.0f, 0.0f), vec3(0.44f, 0.0f, 0.0f),
		vec3(0.44f, 1.08f, 0.0f), vec3(-0.44f, 1.08f, 0.0f), texDecor3);
	mat4 d3Pos = translate(mat4(1.0f), vec3(stCenterX + 0.05f, sideTableY + 0.002f, stCenterZ + 0.05f));
	d3Pos = rotate(d3Pos, radians(8.0f), vec3(0.0f, 1.0f, 0.0f));
	decor3_2D->transformation(d3Pos);

	// القطعة 3: لوحة سينشي وكونان على يمين الطاولة (حجم أكبر ومباعدة بوضوح عن القوس)
	PolygonWithTexture *decor2_2D = createTexturedQuad(
		vec3(-0.46f, 0.0f, 0.0f), vec3(0.46f, 0.0f, 0.0f),
		vec3(0.46f, 0.96f, 0.0f), vec3(-0.46f, 0.96f, 0.0f), texDecor2);
	mat4 d2Pos = translate(mat4(1.0f), vec3(stCenterX + 1.25f, sideTableY + 0.002f, stCenterZ + 0.08f));
	d2Pos = rotate(d2Pos, radians(-14.0f), vec3(0.0f, 1.0f, 0.0f));
	decor2_2D->transformation(d2Pos);

	// 4 كراسي 3D صلبة حوالين طاولة اللعب من نفس لون الطاولة تماماً
	std::vector<PolygonWithTexture *> roomChairs;
	vec3 chairSeatCol = vec3(0.45f, 0.30f, 0.20f); // نفس لون سطح الطاولة
	vec3 chairLegCol = vec3(0.32f, 0.20f, 0.13f);  // نفس لون أرجل الطاولة

	float seatY = 0.85f; // ارتفاع مقعد الكرسي متناسب مع ارتفاع الطاولة
	float seatW = 0.68f, seatD = 0.64f, seatThick = 0.08f;
	float backH = 0.72f;

	auto addChair = [&](vec3 pos, float rotAngleRad)
	{
		auto addPart = [&](float w, float h, float d, vec3 localPos, vec3 col)
		{
			auto box = createTexturedBox(w, h, d, texTable, texTable, col);
			mat4 m = translate(mat4(1.0f), pos);
			m = rotate(m, rotAngleRad, vec3(0.0f, 1.0f, 0.0f));
			m = translate(m, localPos);
			for (auto *f : box)
				f->transformation(m);
			roomChairs.insert(roomChairs.end(), box.begin(), box.end());
		};

		// 1. مقعد الكرسي (3D بـ 6 وجوه)
		addPart(seatW, seatThick, seatD, vec3(0.0f, seatY, 0.0f), chairSeatCol);

		// 2. أرجل الكرسي الأربع
		float lx[2] = {-seatW / 2.0f + 0.06f, seatW / 2.0f - 0.06f};
		float lz[2] = {-seatD / 2.0f + 0.06f, seatD / 2.0f - 0.06f};
		float legH = seatY - seatThick / 2.0f;
		for (int i = 0; i < 2; i++)
			for (int j = 0; j < 2; j++)
				addPart(0.07f, legH, 0.07f, vec3(lx[i], legH / 2.0f, lz[j]), chairLegCol);

		// 3. مسند الظهر (قائمان جانبيان + عارضتان أفقيتان من الخلف)
		float backZ = -seatD / 2.0f + 0.04f;
		addPart(0.07f, backH, 0.07f, vec3(lx[0], seatY + backH / 2.0f, backZ), chairLegCol);
		addPart(0.07f, backH, 0.07f, vec3(lx[1], seatY + backH / 2.0f, backZ), chairLegCol);
		addPart(seatW, 0.14f, 0.06f, vec3(0.0f, seatY + backH - 0.10f, backZ), chairSeatCol);
		addPart(seatW, 0.10f, 0.06f, vec3(0.0f, seatY + backH * 0.45f, backZ), chairSeatCol);
	};

	// توزيع الكراسي الأربعة حول الطاولة (أمام، خلف، يمين، يسار) مسند الظهر للخارج
	float distZ = 1.40f;
	float distX = 1.95f;
	addChair(vec3(0.0f, 0.0f, distZ), radians(180.0f)); // كرسي أمامي يواجه الطاولة
	addChair(vec3(0.0f, 0.0f, -distZ), radians(0.0f));	// كرسي خلفي يواجه الطاولة
	addChair(vec3(-distX, 0.0f, 0.0f), radians(90.0f)); // كرسي يساري يواجه الطاولة
	addChair(vec3(distX, 0.0f, 0.0f), radians(-90.0f)); // كرسي يميني يواجه الطاولة

	ourShader.use();
	mat4 projection = perspective(radians(45.0f), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
	ourShader.setMat4("projection", projection);

	while (!glfwWindowShouldClose(window))
	{
		float currentFrame = static_cast<float>(glfwGetTime());
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		processInput(window);
		if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS && !rPressed)
		{
			rPressed = true;
			for (int i = 0; i < cards.size(); i++)
			{
				cards[i].currentPos = cards[i].deckPos;
				cards[i].moveProgress = 0.0f;
				cards[i].isPlayed = false;
			}
		}
		if (glfwGetKey(window, GLFW_KEY_R) == GLFW_RELEASE)
		{
			rPressed = false;
		}
		if (!freeCameraMode)
		{
			float camSpeed = clamp(deltaTime * 4.5f, 0.0f, 1.0f);
			camera.Position = mix(camera.Position, targetCamPos, camSpeed);
			camera.Yaw = mix(camera.Yaw, targetYaw, camSpeed);
			camera.Pitch = mix(camera.Pitch, targetPitch, camSpeed);

			vec3 front;
			front.x = cos(radians(camera.Yaw)) * cos(radians(camera.Pitch));
			front.y = sin(radians(camera.Pitch));
			front.z = sin(radians(camera.Yaw)) * cos(radians(camera.Pitch));
			camera.Front = normalize(front);
			camera.Right = normalize(cross(camera.Front, camera.WorldUp));
			camera.Up = normalize(cross(camera.Right, camera.Front));
		}

		float standSpeed = clamp(deltaTime * 5.0f, 0.0f, 1.0f);
		currentStandAngle = mix(currentStandAngle, targetStandAngle, standSpeed);

		for (int i = 0; i < 5; i++)
		{
			if (cards[i].isPlayed && cards[i].moveProgress < 1.0f)
			{
				cards[i].moveProgress += deltaTime * 1.8f;
				if (cards[i].moveProgress > 1.0f)
					cards[i].moveProgress = 1.0f;
			}
			float t = cards[i].moveProgress;
			cards[i].currentPos = mix(cards[i].deckPos, cards[i].playedPos, t);
			cards[i].currentPos.y += sin(t * 3.14159f) * 0.22f;
		}

		glClearColor(0.10f, 0.10f, 0.12f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		mat4 view = camera.GetViewMatrix();
		ourShader.setMat4("view", view);

		// 1. رسم الغرفة والطاولة والبطاقات بدون تفريغ (useDiscard = false)
		ourShader.setBool("useDiscard", false);
		floorQuad->draw(ourShader);
		backWall->draw(ourShader);
		frontWall->draw(ourShader);
		leftWall->draw(ourShader);
		rightWall->draw(ourShader);
		ceilingQuad->draw(ourShader);

		tornPoster->draw(ourShader);
		for (auto *sc : sideCardStack)
			sc->draw(ourShader);
		for (auto *f : tableTop)
			f->draw(ourShader);
		for (auto &leg : tableLegs)
			for (auto *f : leg)
				f->draw(ourShader);
		for (auto *f : roomChairs)
			f->draw(ourShader);
		secOpponent->draw(ourShader);
		secPlayed->draw(ourShader);
		secDeck->draw(ourShader);

		// رسم البطاقات الخمسة كاملة بدون أي خرق
		for (int i = 0; i < 5; i++)
		{
			mat4 model = mat4(1.0f);
			model = translate(model, cards[i].currentPos);
			float effectiveAngle = cards[i].isPlayed ? 0.0f : currentStandAngle;
			model = rotate(model, radians(effectiveAngle), vec3(1.0f, 0.0f, 0.0f));

			cards[i].mesh->transformation(model);
			cards[i].mesh->draw(ourShader);
		}

		// 2. رسم الطاولة الجانبية والمجسم الـ 3D الصلب الثابت
		ourShader.setBool("useDiscard", false);
		for (auto *f : sideTableTop)
			f->draw(ourShader);
		for (auto *f : sideTableLegs)
			f->draw(ourShader);
		for (auto *f : lantern3D)
			f->draw(ourShader);

		// 3. تفعيل التفريغ لقطعتي الديكور 2D فقط
		ourShader.setBool("useDiscard", true);
		decor2_2D->draw(ourShader);
		decor3_2D->draw(ourShader);
		ourShader.setBool("useDiscard", false);

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	glfwTerminate();
	return 0;
}

void processInput(GLFWwindow *window)
{
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);

	if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS)
	{
		freeCameraMode = false;
		glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
		cameraState = 1;
		targetCamPos = camPos1;
		targetYaw = yaw1;
		targetPitch = pitch1;
		targetStandAngle = 0.0f;
	}

	if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS)
	{
		freeCameraMode = false;
		glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
		cameraState = 2;
		targetCamPos = camPos2;
		targetYaw = yaw2;
		targetPitch = pitch2;
		targetStandAngle = 68.0f;
	}

	bool cPressed = (glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS);
	if (cPressed && !cKeyWasPressed)
	{
		freeCameraMode = !freeCameraMode;
		if (freeCameraMode)
		{
			firstMouse = true;
			glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
		}
		else
		{
			glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
		}
	}
	cKeyWasPressed = cPressed;

	if (freeCameraMode)
	{
		if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
			camera.ProcessKeyboard(FORWARD, deltaTime);
		if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
			camera.ProcessKeyboard(BACKWARD, deltaTime);
		if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
			camera.ProcessKeyboard(LEFT, deltaTime);
		if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
			camera.ProcessKeyboard(RIGHT, deltaTime);
	}

	bool gPressed = (glfwGetKey(window, GLFW_KEY_G) == GLFW_PRESS);
	if (gPressed && !gKeyWasPressed)
	{
		std::vector<int> availableIndices;
		for (int i = 0; i < 5; i++)
		{
			if (!cards[i].isPlayed)
				availableIndices.push_back(i);
		}

		if (!availableIndices.empty())
		{
			int randomPick = availableIndices[rand() % availableIndices.size()];
			cards[randomPick].isPlayed = true;
		}
	}
	gKeyWasPressed = gPressed;
}

void mouse_callback(GLFWwindow *window, double xposIn, double yposIn)
{
	if (!freeCameraMode)
		return;

	float xpos = static_cast<float>(xposIn);
	float ypos = static_cast<float>(yposIn);

	if (firstMouse)
	{
		lastX = xpos;
		lastY = ypos;
		firstMouse = false;
	}

	float xoffset = xpos - lastX;
	float yoffset = lastY - ypos;

	lastX = xpos;
	lastY = ypos;

	camera.ProcessMouseMovement(xoffset, yoffset);
}

void framebuffer_size_callback(GLFWwindow *window, int width, int height)
{
	glViewport(0, 0, width, height);
}
