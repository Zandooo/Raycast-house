#include "raylib.h"
#include <algorithm>
#include <cmath>
#include <vector>

//Constants

const int W = 1280;
const int H = 720;
const int MAP_W = 11;
const int MAP_H = 17;
const int FLOORS = 2;

const double MOVE_SPEED = 3.0;
const double TURN_SPEED = 2.0;
const double PLAYER_R = 0.2;

//Map

const char* LAYOUT[FLOORS][MAP_H] = {
{
	"11111111111",
	"1.....3..11",
	"1.....3..11",
	"1.....3..11",
	"1.....3.311",
	"1........11",
	"122......11",
	"1........11",
	"1........11",
	"1........11",
	"1........11",
	"1........11",
	"1........11",
	"1........11",
	"1........11",
	"1........11",
	"11111111111",
},
{ 
    "11111111111",
    "1.........1",   
    "1.........1",
    "1.........1",
    "1.........1",
    "1.....2..21",   
    "1222222...1",   
    "1.....2...1",
    "1.........1",
    "1.....3...1",
    "1333333...1",   
    "1.........1",   
    "1.....3...1",   
    "1.....3...1",
    "11111111111",   
    "11111111111",   
    "11111111111",
}
};

int worldMap[FLOORS][MAP_H][MAP_W];
int floorNum = 0;

void loadMaps(){
	for (int f = 0; f<FLOORS; f++){
		for (int y = 0; y<MAP_H; y++){
			for (int x = 0; x<MAP_W; x++){
				char c = LAYOUT[f][y][x];
				worldMap[f][y][x] = (c == '.') ? 0 : (c - '0');
			}
		}
	}
}

//Wall Colors

Color wallColor(int id, bool darker) {
	Color c;
	switch (id) {
		case 1: c = Color{205, 200, 190, 255}; break;
		case 2: c = Color{190, 130, 100, 255}; break;
		default: c = Color{110, 150, 190, 255}; break;
	}
	if (darker) { c.r /= 2; c.g /= 2; c.b /= 2;}
	return c;
}

struct Sprite {
	double x,y;
	Texture2D tex;
	double scale;
	double hw, hh;
	bool solid;
};
std::vector<Sprite> sprites[FLOORS];

//secret easter egg video gemini loves you

void addSprite(int floor, double x, double y, Texture2D tex, double scale, double hw, double hh, bool solid = true){
	sprites[floor].push_back(Sprite{x, y, tex, scale, hw, hh, solid});
}

std::vector<Texture2D> allTextures;

Texture2D loadOrPlaceholder(const char* path, Color color, int w, int h){
	Texture2D t;
	if (FileExists(path)) {
		t = LoadTexture(path);
	} else {
		Image img = GenImageColor(w, h, BLACK);
		ImageDrawRectangle(&img, 3, 3, w - 6, h - 6, color);
		t = LoadTextureFromImage(img);
		UnloadImage(img);
	}
	allTextures.push_back(t);
	return t;
}

//Stairs
struct Stair {
	int floor;
	double x, y;
	int toFloor;
	double toX, toY;
};
std::vector<Stair> stairs;

struct MemoryZone {
	int floor;
	double x0, y0, x1, y1;
	const char* text;
};
std::vector<MemoryZone> zones;

//Collisions
bool isBlocked(double x, double y){
	for (int dy = -1; dy <= 1; dy += 2){
		for (int dx = -1; dx <= 1; dx += 2){
			double px = x + dx * PLAYER_R;
			double py = y + dy * PLAYER_R;
			if (px < 0 || py < 0 || px >= MAP_W || py >= MAP_H) return true;
			if (worldMap[floorNum][(int)py][(int)px] != 0) return true;
		}
	}
	for (const Sprite& s : sprites[floorNum]) {
		if (!s.solid) continue;
		if(fabs(x - s.x) < s.hw + PLAYER_R && fabs(y - s.y) < s.hh + PLAYER_R) return true;
	}
	return false;
}

//Minimap
void drawMinimap(double posX, double posY, double dirX, double dirY){
	const int S = 10;
	const int ox = 10, oy = 10;
	DrawRectangle(ox - 2, oy - 2, MAP_W * S + 4, MAP_H * S + 4, Fade(BLACK, 0.6f));
	for(int y = 0; y < MAP_H; y++){
		for(int x = 0; x < MAP_W; x++){
			int id = worldMap[floorNum][y][x];
			if (id != 0) DrawRectangle(ox + x * S, oy + y * S, S, S, wallColor(id, false));
		}
	}
	for (const Sprite& s : sprites[floorNum]) {
		DrawRectangle(ox + (int)(s.x * S) - 2, oy + (int)(s.y * S) - 2, 4, 4, ORANGE);
	}
	for (const Stair& st : stairs){
		if (st.floor == floorNum) DrawRectangle(ox + (int)(st.x * S) - 3, oy + (int)(st.y * S) - 3, 6, 6, YELLOW);
	}
	DrawCircle(ox + (int)(posX * S), oy + (int)(posY * S), 3, RED);
	DrawLine(ox + (int)(posX * S), oy + (int)(posY * S), ox + (int)((posX + dirX) * S), oy + (int)((posY + dirY) * S), RED);
}

int main(){
	InitWindow(W, H, "Home");
	SetTargetFPS(60);
	
	loadMaps();
	
	
	Texture2D texKitchenCounterLong = loadOrPlaceholder("assets/kitchen_counter_long.png", Color{90,170,90, 255}, 335, 24);
	Texture2D texKitchenCounterCorner = loadOrPlaceholder("assets/kitchen_counter_corner.png", Color{90,170,90, 255}, 128, 24);
	Texture2D texKitchenCounterSmall = loadOrPlaceholder("assets/kitchen_counter_small.png", Color{90,170,90, 255}, 94, 24);
	Texture2D texBathroomCounterSmall = loadOrPlaceholder("assets/bathroom_counter_small.png", Color{150,90,170, 255}, 84, 24);
	Texture2D texToilet = loadOrPlaceholder("assets/toilet.png", Color{235,235,235, 255}, 124, 24);
	Texture2D texStairsUp = loadOrPlaceholder("assets/stairs_up.png", Color{230,200,70, 255}, 47, 24);
	Texture2D texHeater = loadOrPlaceholder("assets/heater.png", Color{70,90,200, 255}, 149, 24);
	Texture2D texTable = loadOrPlaceholder("assets/table.png", Color{160,110,70, 255}, 402, 24);
	Texture2D texSofa = loadOrPlaceholder("assets/sofa.png", Color{140,30,45, 255}, 266, 24);
	Texture2D texKingBed = loadOrPlaceholder("assets/king_bed.png", Color{150,30,45, 255}, 349, 24);
	Texture2D texWardrobe = loadOrPlaceholder("assets/wardrobe.png", Color{185,125,85, 255}, 182, 24);
	Texture2D texSingleBed = loadOrPlaceholder("assets/single_bed.png", Color{170,50,60, 255}, 380, 24);
	Texture2D texShower = loadOrPlaceholder("assets/shower.png", Color{170,130,200, 255}, 40, 24);
	Texture2D texBathroomCounter = loadOrPlaceholder("assets/bathroom_counter.png", Color{150,90,170, 255}, 149, 24);
	Texture2D texStairsDown = loadOrPlaceholder("assets/stairs_down.png", Color{200,150,50, 255}, 52, 24);
	Texture2D texDad = loadOrPlaceholder("assets/dad.png", Color{60,90,160, 255}, 28, 72);
	Texture2D texMom = loadOrPlaceholder("assets/mom.png", Color{190,90,130, 255}, 28, 72);
	
	//1st floor
	addSprite(0, 3.40, 1.70, texKitchenCounterLong, 0.36, 2.07, 0.51);
	addSprite(0, 1.90, 3.20, texKitchenCounterCorner, 0.36, 0.58, 0.79);
	addSprite(0, 5.00, 3.20, texKitchenCounterSmall, 0.36, 0.42, 0.58);
	addSprite(0, 8.20, 1.70, texBathroomCounterSmall, 0.36, 0.52, 0.43);
	addSprite(0, 8.20, 3.20, texToilet, 0.30, 0.52, 0.64);
	addSprite(0, 2.00, 4.80, texStairsUp, 0.90, 0.72, 0.66, false);
	addSprite(0, 1.90, 7.90, texHeater, 0.30, 0.77, 0.67);
	addSprite(0, 7.10, 8.80, texTable, 0.32, 1.33, 2.21);
	addSprite(0, 2.00, 12.60, texSofa, 0.40, 0.80, 1.83);
	addSprite(0, 3.50, 12.60, texDad, 0.70, 0.30, 0.30);
	addSprite(0, 3.50, 3.00, texMom, 0.70, 0.30, 0.30);
	
	//2nd floor
	addSprite(1, 4.60, 2.60, texKingBed, 0.30, 1.80, 1.45);
	addSprite(1, 9.30, 2.60, texSingleBed, 0.22, 0.51, 1.41);
	addSprite(1, 3.50, 5.20, texWardrobe, 0.80, 2.50, 0.51);
	addSprite(1, 3.54, 7.45, texWardrobe, 0.79, 2.46, 0.40);
	addSprite(1, 3.00, 9.00, texSingleBed, 0.28, 1.83, 0.57);
	addSprite(1, 1.70, 11.60, texToilet, 0.24, 0.52, 0.51);
	addSprite(1, 1.80, 13.30, texShower, 0.85, 0.59, 0.54);
	addSprite(1, 4.10, 11.60, texBathroomCounter, 0.36, 1.11, 0.48); 
	addSprite(1, 4.20, 13.30, texBathroomCounter, 0.35, 1.08, 0.50);
	addSprite(1, 9.10, 12.90, texStairsDown, 0.90, 0.79, 0.81, false);
	
	stairs.push_back(Stair{0, 2.0, 4.8, 1, 7.6, 12.4});
	stairs.push_back(Stair{1, 9.1, 12.9, 0, 3.6, 5.0});
	
	zones.push_back(MemoryZone{0, 7, 1, 9, 5, "Bathroom"});
	zones.push_back(MemoryZone{0, 1, 1, 6, 6, "Kitchen"});
	zones.push_back(MemoryZone{0, 1, 1, 9, 16, "Living Room"});
	zones.push_back(MemoryZone{1, 1, 11, 6, 14, "Bathroom"});
	zones.push_back(MemoryZone{1, 1, 7, 6, 10, "Bedroom 2"});
	zones.push_back(MemoryZone{1, 1, 1, 10, 6, "Bedroom 1"});
	zones.push_back(MemoryZone{1, 1, 1, 10, 14, "Hallway"});
	
	//player
	double posX = 5.7, posY = 14.5;
	double dirX = 0.0, dirY = -1.0;
	double planeX = 0.66, planeY = 0.0;
	
	double cooldown = 0.0;
	double fade = 0.0;
	bool showMap = true;
	
	std::vector<double> zBuffer(W);
	
	while (!WindowShouldClose()) {
		double dt = GetFrameTime();
		
		//Input
		double move = 0;
		if (IsKeyDown(KEY_W)) move += MOVE_SPEED * dt;
		if (IsKeyDown(KEY_S)) move -= MOVE_SPEED * dt;
		if (move != 0) {
			double nx = posX + dirX * move;
			double ny = posY + dirY * move;
			if (!isBlocked(nx, posY)) posX = nx;
			if (!isBlocked(posX, ny)) posY = ny;
		}
		
		
		//Rotation
		double rot = 0;
		if (IsKeyDown(KEY_A)) rot -= TURN_SPEED * dt;
		if (IsKeyDown(KEY_D)) rot += TURN_SPEED * dt;
		if (rot != 0) {
			double c = cos(rot), s = sin(rot);
			double oldDirX = dirX;
			dirX = dirX * c - dirY * s;
			dirY = oldDirX * s + dirY * c;
			double oldPlaneX = planeX;
			planeX = planeX * c - planeY * s;
			planeY = oldPlaneX * s + planeY * c;
		}
		if (IsKeyPressed(KEY_M)) showMap = !showMap;
		
		//Stairs
		cooldown -= dt;
		for (const Stair& st : stairs) {
			if (st.floor == floorNum && cooldown <= 0 && fabs(posX - st.x) < 0.5 && fabs(posY - st.y) < 0.5) {
				floorNum = st.toFloor;
				posX = st.toX;
				posY = st.toY;
				cooldown = 1.0;
				fade = 1.0;
				break;
			}
		}
		fade = std::max(0.0, fade - dt * 2.0);
		
		//Render
		BeginDrawing();
		ClearBackground(BLACK);
		
		DrawRectangle(0, 0, W, H / 2, Color{40, 40, 60, 255});
		DrawRectangle(0, H / 2, W, H / 2, Color{75, 62, 50, 255});
		
		for (int x = 0; x < W; x++) {
			double cameraX = 2.0 * x / W -1.0;
			double rayDirX = dirX + planeX * cameraX;
			double rayDirY = dirY + planeY * cameraX;
			
			int mapX = (int)posX;
			int mapY = (int)posY;
			
			double deltaX = (rayDirX == 0) ? 1e30 : fabs(1.0 / rayDirX);
			double deltaY = (rayDirY == 0) ? 1e30 : fabs(1.0 / rayDirY);
			
			double sideX, sideY;
			int stepX, stepY;
			if (rayDirX < 0) { stepX = -1; sideX = (posX - mapX) * deltaX;}
			else { stepX = 1; sideX = (mapX +1.0 - posX) * deltaX;}
			if (rayDirY < 0) { stepY = -1; sideY = (posY - mapY) * deltaY;}
			else { stepY = 1; sideY = (mapY + 1.0 - posY) * deltaY;}
			
			bool hitSide = false;
			while (worldMap[floorNum][mapY][mapX] == 0){
				if(sideX < sideY) 	{ sideX += deltaX; mapX += stepX; hitSide = false;}
				else 				{ sideY += deltaY; mapY += stepY; hitSide = true;}
			}
			
			double dist = hitSide ? (sideY - deltaY) : (sideX - deltaX);
			zBuffer[x] = dist;
			
			int lineH = (int)(H / dist);
			int y0 = std::max(0, H / 2 - lineH / 2);
			int y1 = std::min(H - 1, H / 2 + lineH / 2);
			DrawRectangle(x, y0, 1, y1 - y0 + 1, wallColor(worldMap[floorNum][mapY][mapX], hitSide));
		}
		
		std::vector<Sprite>& list = sprites[floorNum];
		std::sort(list.begin(), list.end(), [&](const Sprite& a, const Sprite& b) {
			double da = (a.x - posX) * (a.x - posX) + (a.y - posY) * (a.y - posY);
			double db = (b.x - posX) * (b.x - posX) + (b.y - posY) * (b.y - posY);
			return da > db;	
		});
		
		for (const Sprite& s : list) {
			double sx = s.x - posX;
			double sy = s.y - posY;
			
			double invDet = 1.0 / (planeX * dirY - dirX * planeY);
			double tx = invDet * (dirY * sx - dirX * sy);
			double ty = invDet * (-planeY * sx + planeX * sy);
			if (ty < 0.1) continue;
			
			int screenX = (int)((W/2) * (1.0 + tx/ty));
			int wallH = (int)(H / ty);
			int h = (int)(wallH * s.scale);
			int w = (int)(h * (double)s.tex.width / s.tex.height);
			int top = H / 2 + wallH / 2 - h;
			int startX = screenX - w/2;
			
			for (int stripe = std::max(0, startX); stripe < std::min(W, startX + w); stripe++) {
				if (ty < zBuffer[stripe]){
					int texX = (stripe - startX) * s.tex.width / w;
					DrawTexturePro(s.tex, Rectangle{(float)texX, 0.0f, 1.0f, (float)s.tex.height}, Rectangle{(float)stripe, (float)top, 1.0f, (float)h}, Vector2{0,0}, 0.0f, WHITE);
				}
			}
		}
		
		for (const MemoryZone& z : zones) {
			if (z.floor == floorNum && posX >= z.x0 && posX <= z.x1 && posY >= z.y0 && posY <= z.y1) {
				DrawRectangle(40, H - 90, W - 80, 60, Fade(BLACK, 0.7f));
				DrawText(z.text, 55, H - 70, 20, WHITE);
				break;
			}
		}
		
		//HUD
		if (showMap) drawMinimap(posX, posY, dirX, dirY);
		DrawText(floorNum == 0 ? "Floor 1" : "Floor 2", W - 110, 12, 20, WHITE);
		if (fade > 0) DrawRectangle(0, 0, W, H, Fade(BLACK, (float)fade));
		
		EndDrawing();
	}
	for (Texture2D& t : allTextures) UnloadTexture(t);
	CloseWindow();
	return 0;	
}