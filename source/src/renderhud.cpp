// renderhud.cpp: HUD rendering

#include "pch.h"
#include "cube.h"

inline void drawicon(Texture *tex, float x, float y, float s, int col, int row, float ts)
{
	if(tex && tex->xs == tex->ys) quad(tex->id, x, y, s, ts*col, ts*row, ts);
}

void drawequipicon(float x, float y, int col, int row, int pulse, playerent *p = NULL) // pulse: 3 = (1 - pulse) | (2 - blue regen)
{
	static Texture *tex = textureload("packages/misc/items.png", 3);
	if(tex)
	{
		glEnable(GL_BLEND);
		float cfade = (pulse&2) && p ? (lastmillis-p->lastregen)/1000.f : 1.f;
		glColor4f(cfade, cfade, cfade * 2, (pulse&1) ? (0.2f+(sinf(lastmillis/100.0f)+1.0f)/2.0f) : 1.f);
		drawicon(tex, x, y, 120, col, row, 1/5.0f);
		glDisable(GL_BLEND);
	}
}

void drawradaricon(float x, float y, float s, int col, int row)
{
	static Texture *tex = textureload("packages/misc/radaricons.png", 3);
	if(tex)
	{
		glEnable(GL_BLEND);
		drawicon(tex, x, y, s, col, row, 1/4.0f);
		glDisable(GL_BLEND);
	}
}

void drawflagicons(const flaginfo &f, playerent *p)
{
	static Texture *ctftex = textureload("packages/misc/ctficons.png", 3),
					*hktftex = textureload("packages/misc/hktficons.png", 3),
					*flagtex = textureload("packages/misc/flagicons.png", 3);
	if(flagtex)
	{
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glColor4f(1, 1, 1,
			f.state == CTFF_INBASE ? .2f :
			f.state == CTFF_IDLE ? .1f :
			f.actor == p && f.state == CTFF_STOLEN ? (sinf(lastmillis/100.0f)+1.0f) / 2.0f :
			1
		);
		// CTF
		int row = 0;
		// HTF
		if(m_hunt(gamemode)) row = 1;
		// KTF
		else if(m_keep(gamemode)) row = 2;
		drawicon(flagtex, f.team*120+VIRTW/4.0f*3.0f, 1650, 120, f.team, row, 1/3.f);
	}
	// Must be stolen for big flag-stolen icon
	if(f.state != CTFF_STOLEN) return;
	// CTF OR KTF2/Returner
	int row = (m_capture(gamemode) || (m_ktf2(gamemode, mutators) && m_team(gamemode, mutators))) && f.actor && f.actor->team == f.team ? 1 : 0;
	// HTF + KTF
	if(m_keep(gamemode) && !(m_ktf2(gamemode, mutators) && m_team(gamemode, mutators))) row = 1;
	// pulses
	glColor4f(1, 1, 1, f.actor == p ? (sinf(lastmillis/100.0f)+1.0f) / 2.0f : .6f);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE);
	drawicon((m_capture(gamemode) || (m_ktf2(gamemode, mutators) && m_team(gamemode, mutators))) ? ctftex : hktftex, VIRTW - 225 * (!f.team && flaginfos[1].state != CTFF_STOLEN ? 1 : 2 - f.team) - 10, VIRTH*5/8, 225, f.team, row, 1/2.f);
}

void drawvoteicon(float x, float y, int col, int row, bool noblend)
{
	static Texture *tex = textureload("packages/misc/voteicons.png", 3);
	if(tex)
	{
		if(noblend) glDisable(GL_BLEND);
		drawicon(tex, x, y, 240, col, row, 1/2.0f);
		if(noblend) glEnable(GL_BLEND);
	}
}

VARP(crosshairsize, 0, 15, 50);
VARP(crosshairreddotsize, 0, 15, 50);
VARP(crosshairreddottreshold, 0, 1001, 1001);
VARP(hidestats, 0, 1, 2); // hardcore does not override
VARP(hideobits, 0, 0, 1);
VARP(hideradar, 0, 0, 1);
VARP(hidecompass, 0, 0, 1);
VARP(hideteam, 0, 0, 1);
//VARP(radarres, 1, 64, 1024); // auto
VARP(radarentsize, 1, 6, 64);
VARP(hidectfhud, 0, 0, 1); // hardcore does not override
VARP(hidevote, 0, 0, 2); // hardcore MUST not override
VARP(showstreak, 0, 1, 1);
VARP(hidehudmsgs, 0, 0, 1);
VARP(hidehudequipment, 0, 0, 1);
VARP(hideconsole, 0, 0, 1); // hardcore MUST not override ("important" information...)
VARP(hidespecthud, 0, 0, 1); // hardcore does not override
VARP(hidehardcore, 0, 2, 5); // (VARP later) 0: no change, 1: -non-essential (streak meter, team - see scoreboard!, perk, distance, health), 2: -crosshair, hitmarker, 3: -ammo count, 4: -killfeed, 5: -radar
#define show_hud_element(setting, hardcorelevel) (setting && (!m_real(gamemode, mutators) || hidehardcore < hardcorelevel))
VAR(showmap, 0, 0, 1);

void drawscope()
{
	// this may need to change depending on the aspect ratio at which the scope image is drawn at
	const float scopeaspect = 4.0f/3.0f;

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	static Texture *scopetex = textureload("packages/misc/scope.png", 3);
	glBindTexture(GL_TEXTURE_2D, scopetex->id);
	glBegin(GL_QUADS);
	glColor3ub(255,255,255);

	// figure out the bounds of the scope given the desired aspect ratio
	float w = min(scopeaspect*VIRTH, float(VIRTW)),
		  x1 = VIRTW/2 - w/2,
		  x2 = VIRTW/2 + w/2;

	glTexCoord2f(0, 0); glVertex2f(x1, 0);
	glTexCoord2f(1, 0); glVertex2f(x2, 0);
	glTexCoord2f(1, 1); glVertex2f(x2, VIRTH);
	glTexCoord2f(0, 1); glVertex2f(x1, VIRTH);

	// fill unused space with border texels
	if(x1 > 0)
	{
		glTexCoord2f(0, 0); glVertex2f(0, 0);
		glTexCoord2f(0, 0); glVertex2f(x1, 0);
		glTexCoord2f(0, 1); glVertex2f(x1, VIRTH);
		glTexCoord2f(0, 1); glVertex2f(0, VIRTH);
	}

	if(x2 < VIRTW)
	{
		glTexCoord2f(1, 0); glVertex2f(x2, 0);
		glTexCoord2f(1, 0); glVertex2f(VIRTW, 0);
		glTexCoord2f(1, 1); glVertex2f(VIRTW, VIRTH);
		glTexCoord2f(1, 1); glVertex2f(x2, VIRTH);
	}

	glEnd();
}

const char *crosshairnames[CROSSHAIR_NUM] = { "default", "scope", "shotgun", "vertical", "horizontal", "hit", "reddot" };
Texture *crosshairs[CROSSHAIR_NUM] = { NULL }; // weapon specific crosshairs

Texture *loadcrosshairtexture(const char *c, int type = -1)
{
	defformatstring(p)("packages/misc/crosshairs/%s", c);
	Texture *crosshair = textureload(p, 3);
	if(crosshair==notexture){
		formatstring(p)("packages/misc/crosshairs/%s", crosshairnames[type < 0 || type >= CROSSHAIR_NUM ? CROSSHAIR_DEFAULT : type]);
		crosshair = textureload(p, 3);
	}
	return crosshair;
}

void loadcrosshair(char *c, char *name)
{
	int n = -1;
	loopi(CROSSHAIR_NUM) if(!strcmp(crosshairnames[i], name)) { n = i; break; }
	if(n<0){
		n = atoi(name);
		if(n<0 || n>=CROSSHAIR_NUM) return;
	}
	crosshairs[n] = loadcrosshairtexture(c, n);
}

COMMAND(loadcrosshair, ARG_2STR);

void drawcrosshair(playerent *p, int n, int teamtype, color *c, float size)
{
	if(!show_hud_element(crosshairsize, 2) || intermission) return;
	Texture *crosshair = crosshairs[n];
	if(!crosshair)
	{
		crosshair = crosshairs[CROSSHAIR_DEFAULT];
		if(!crosshair) crosshair = crosshairs[CROSSHAIR_DEFAULT] = loadcrosshairtexture("default.png");
	}

	static color col;
	col.r = col.b = col.g = col.alpha = 1.f;
	if(c) col = color(c->r, c->g, c->b);
	else if(teamtype)
	{
		if(teamtype == 1) col = color(0.f, 1.f, 0.f);
		else if(teamtype == 2) col = color(1.f, 0.f, 0.f);
	}
	else if(!m_insta(gamemode, mutators)){
		if(p->health<=50 * HEALTHSCALE) col = color(0.5f, 0.25f, 0.f); // orange-red
		if(p->health<=25 * HEALTHSCALE) col = color(0.5f, 0.125f, 0.f); // red-orange
	}
	if(n == CROSSHAIR_DEFAULT) col.alpha = 1.f + p->weaponsel->dynspread() / -1200.f;
	if(n != CROSSHAIR_SCOPE && p->ads) col.alpha *= 1 - sqrtf(p->ads * (n == CROSSHAIR_SHOTGUN ? 0.5f : 1)) / sqrtf(600);
	float usz = (float)crosshairsize, chsize = size>0 ? size : usz;
	glColor4f(col.r, col.g, col.b, col.alpha * 0.8f);
	if(n == CROSSHAIR_DEFAULT){
		usz *= 3.5f;
		float ct = usz / 1.8f;
		chsize = p->weaponsel->dynspread() * 100 * (p->perk2 == PERK2_STEADY ? .65f : 1) / dynfov();
		if(isthirdperson) chsize *= worldpos.dist(focus->o)/worldpos.dist(camera1->o);
		if(m_classic(gamemode, mutators)) chsize *= .6f;

		Texture *cv = crosshairs[CROSSHAIR_V], *ch = crosshairs[CROSSHAIR_H];
		if(!cv) cv = textureload("packages/misc/crosshairs/vertical.png", 3);
		if(!ch) ch = textureload("packages/misc/crosshairs/horizontal.png", 3);
		
		/*if(ch->bpp==32) glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		else */glBlendFunc(GL_SRC_ALPHA, GL_ONE);
		glBindTexture(GL_TEXTURE_2D, ch->id);

		glBegin(GL_QUADS);
		// top
		glTexCoord2f(0, 0); glVertex2f(VIRTW/2 - ct, VIRTH/2 - chsize - usz);
		glTexCoord2f(1, 0); glVertex2f(VIRTW/2 + ct, VIRTH/2 - chsize - usz);
		glTexCoord2f(1, 1); glVertex2f(VIRTW/2 + ct, VIRTH/2 - chsize);
		glTexCoord2f(0, 1); glVertex2f(VIRTW/2 - ct, VIRTH/2 - chsize);
		// bottom
		glTexCoord2f(1, 1); glVertex2f(VIRTW/2 - ct, VIRTH/2 + chsize);
		glTexCoord2f(0, 1); glVertex2f(VIRTW/2 + ct, VIRTH/2 + chsize);
		glTexCoord2f(0, 0); glVertex2f(VIRTW/2 + ct, VIRTH/2 + chsize + usz);
		glTexCoord2f(1, 0); glVertex2f(VIRTW/2 - ct, VIRTH/2 + chsize + usz);

		// change texture
		glEnd();
		/*if(cv->bpp==32) glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		else */glBlendFunc(GL_SRC_ALPHA, GL_ONE);
		glBindTexture(GL_TEXTURE_2D, cv->id);

		glBegin(GL_QUADS);
		// left
		glTexCoord2f(0, 0); glVertex2f(VIRTW/2 - chsize - usz, VIRTH/2 - ct);
		glTexCoord2f(1, 0); glVertex2f(VIRTW/2 - chsize, VIRTH/2 - ct);
		glTexCoord2f(1, 1); glVertex2f(VIRTW/2 - chsize, VIRTH/2 + ct);
		glTexCoord2f(0, 1); glVertex2f(VIRTW/2 - chsize - usz, VIRTH/2 + ct);
		// right
		glTexCoord2f(1, 1); glVertex2f(VIRTW/2 + chsize, VIRTH/2 - ct);
		glTexCoord2f(0, 1); glVertex2f(VIRTW/2 + chsize + usz, VIRTH/2 - ct);
		glTexCoord2f(0, 0); glVertex2f(VIRTW/2 + chsize + usz, VIRTH/2 + ct);
		glTexCoord2f(1, 0); glVertex2f(VIRTW/2 + chsize, VIRTH/2 + ct);
	}
	else{
		if(n == CROSSHAIR_SHOTGUN){
			chsize = p->weaponsel->dynspread() * 100 * (p->perk2 == PERK2_STEADY ? .75f : 1) / dynfov();
			if(isthirdperson) chsize *= worldpos.dist(focus->o)/worldpos.dist(camera1->o);
			if(m_classic(gamemode, mutators)) chsize *= .75f;
		}

		if(crosshair->bpp==32) glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		else glBlendFunc(GL_SRC_ALPHA, GL_ONE);
		glBindTexture(GL_TEXTURE_2D, crosshair->id);

		glBegin(GL_QUADS);
		glTexCoord2f(0, 0); glVertex2f(VIRTW/2 - chsize, VIRTH/2 - chsize);
		glTexCoord2f(1, 0); glVertex2f(VIRTW/2 + chsize, VIRTH/2 - chsize);
		glTexCoord2f(1, 1); glVertex2f(VIRTW/2 + chsize, VIRTH/2 + chsize);
		glTexCoord2f(0, 1); glVertex2f(VIRTW/2 - chsize, VIRTH/2 + chsize);
	}
	glEnd();
}

void drawequipicons(playerent *p)
{
	if(p->state == CS_DEAD || p->state == CS_EDITING || !show_hud_element(!hidehudequipment, 3))
		return;
	glDisable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// health & armor
	if(show_hud_element(!hidehudequipment, 1)){
	/*
	if(p->armor)
		if(p->armor > 25) drawequipicon(560, 1650, (p->armor - 25) / 25, 2, 0);
		else drawequipicon(560, 1650, 3, 3, 0);
	drawequipicon(20, 1650, 2, 3, (lastmillis - p->lastregen < 1000 ? 2 : 0) | ((p->state!=CS_DEAD && p->health<=35*HEALTHSCALE && m_regen(gamemode, mutators)) ? 1 : 0), p);
	*/
	int hc = 0, hr = 3;
	if(p->armor)
	{
		hr = 2;
		if(p->armor >= 100) hc = 4;
		else if(p->armor >= 75) hc = 3;
		else if(p->armor >= 50) hc = 2;
		else if(p->armor >= 25) hc = 1;
		else hc = 0;
	}
	drawequipicon(20, 1650, hc, hr, (lastmillis - p->lastregen < 1000 ? 2 : 0) | ((p->state!=CS_DEAD && p->health<=35*HEALTHSCALE && m_regen(gamemode, mutators)) ? 1 : 0), p);
	}

	// grenades
	int equipx = 0;
	loopi(min(3, p->mag[WEAP_GRENADE])) drawequipicon(1020 + equipx++ * 25, 1650, 1, 1, 0);
	loopi(min(3, p->ammo[WEAP_KNIFE])) drawequipicon(1060 + equipx++ * 30, 1650, 0, 0, 0);

	// weapons
	if(p->weaponsel && p->weaponsel->type>=WEAP_KNIFE && p->weaponsel->type<WEAP_MAX){
		int c = p->weaponsel->type, r = 0;
		if(c == WEAP_GRENADE){// draw nades separately
			if(p->prevweaponsel && p->prevweaponsel->type != WEAP_GRENADE) c = p->prevweaponsel->type;
			else if(p->nextweaponsel && p->nextweaponsel->type != WEAP_GRENADE) c = p->nextweaponsel->type;
			else c = 14; // unknown = HP symbol
		}
		if(c == WEAP_AKIMBO) c = WEAP_PISTOL; // same icon for akimbo & pistol
		switch(c){
			case WEAP_KNIFE: case WEAP_PISTOL: case WEAP_SHOTGUN: case WEAP_SUBGUN: break; // aligned properly
			default: c = 0; break;
			case WEAP_SWORD: c = 0; r = 0; break; // special: sword uses knife
			case WEAP_SNIPER: case WEAP_BOLT: case WEAP_SNIPER2: c = 4; r = 0; break; // special: snipers are shared
			case WEAP_ASSAULT: c = 0; r = 1; break;
			case WEAP_GRENADE: c = 1; r = 1; break;
			case WEAP_HEAL: c = 2; r = 1; break;
			case WEAP_RPG: c = 3; r = 1; break;
			case WEAP_ASSAULT2: c = 4; r = 1; break;
			case WEAP_AKIMBO: c = 1; r = 1; break; // special: pistol and akimbo share
		}
		drawequipicon(560, 1650, c, r, ((!p->weaponsel->ammo || p->weaponsel->mag < magsize(p->weaponsel->type) / 3) && p->weaponsel->type != WEAP_KNIFE && p->weaponsel->type != WEAP_GRENADE && p->weaponsel->type != WEAP_SWORD) ? 1 : 0);
		extern int burst;
		if(p == player1 && burst_weap(p->weaponsel->type))
			drawequipicon(560, 1650, burst ? burst == 1 ? 1 : 2 : 3, 3, 0);
	}
	glEnable(GL_BLEND);
}

void drawradarent(const vec &o, float coordtrans, float yaw, int col, int row, float iconsize, int pulse = 0, float alpha = 1.f, const char *label = NULL, ...)
{
	if(OUTBORD(int(o.x), int(o.y))) return;
	glPushMatrix();
	if(pulse) glColor4f(1.0f, 1.0f, 1.0f, 0.2f+(sinf(lastmillis/30.0f+pulse)+1.0f)/2.0f);
	else glColor4f(1, 1, 1, alpha);
	glTranslatef(o.x * coordtrans, o.y * coordtrans, 0);
	glRotatef(yaw, 0, 0, 1);
	const sqr * const s = S(int(o.x), int(o.y));
	const float scl = 1 + clamp<float>((o.z - s->floor) / (float)(s->ceil - s->floor), 0, 1) * 0.45f;
	drawradaricon(-iconsize/2.0f*scl, -iconsize/2.0f*scl, iconsize*scl, col, row);
	glPopMatrix();
	if(label && showmap)
	{
		glPushMatrix();
		glEnable(GL_BLEND);
		glTranslatef(iconsize/2, iconsize/2, 0);
		glScalef(1/2.0f, 1/2.0f, 1/2.0f);
		s_sprintfdv(lbl, label);
		draw_text(lbl, (int)(o.x * coordtrans *2), (int)(o.y * coordtrans*2), 255, 255, 255, int(alpha * 255));
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glDisable(GL_BLEND);
		glPopMatrix();
	}
}

struct hudline : cline
{
	int type;

	hudline() : type(HUDMSG_INFO) {}
};

struct hudmessages : consolebuffer<hudline>
{
	hudmessages() : consolebuffer<hudline>(20) {}

	void addline(const char *sf)
	{
		if(conlines.length() && conlines[0].type&HUDMSG_OVERWRITE)
		{
			conlines[0].millis = totalmillis;
			conlines[0].type = HUDMSG_INFO;
			copystring(conlines[0].line, sf);
		}
		else consolebuffer<hudline>::addline(sf, totalmillis);
	}
	void editline(int type, const char *sf)
	{
		if(conlines.length() && ((conlines[0].type&HUDMSG_TYPE)==(type&HUDMSG_TYPE) || conlines[0].type&HUDMSG_OVERWRITE))
		{
			conlines[0].millis = totalmillis;
			conlines[0].type = type;
			copystring(conlines[0].line, sf);
		}
		else consolebuffer<hudline>::addline(sf, totalmillis).type = type;
	}
	void render()
	{
		if(!conlines.length()) return;
		glLoadIdentity();
		glOrtho(0, VIRTW*0.9f, VIRTH*0.9f, 0, -1, 1);
		int dispmillis = arenaintermission ? 6000 : 3000;
		loopi(min(conlines.length(), 3)) if(totalmillis-conlines[i].millis<dispmillis)
		{
			cline &c = conlines[i];
			int tw = text_width(c.line);
			draw_text(c.line, int(tw > VIRTW*0.9f ? 0 : (VIRTW*0.9f-tw)/2), int(((VIRTH*0.9f)/4*3)+FONTH*i+pow((totalmillis-c.millis)/(float)dispmillis, 4)*VIRTH*0.9f/4.0f));
		}
	}
};

hudmessages hudmsgs;

void hudoutf(const char *s, ...)
{
	s_sprintfdv(sf, s);
	hudmsgs.addline(sf);
	conoutf("%s", sf);
}

void hudonlyf(const char *s, ...)
{
	s_sprintfdv(sf, s);
	hudmsgs.addline(sf);
}

void hudeditf(int type, const char *s, ...)
{
	s_sprintfdv(sf, s);
	hudmsgs.editline(type, sf);
}

bool insideradar(const vec &centerpos, float radius, const vec &o)
{
	if(showmap) return !o.reject(centerpos, radius);
	return o.distxy(centerpos)<=radius;
}

vec fixradarpos(const vec &o, const vec &centerpos, float res, bool skip = false){
	if(!skip && !showmap && !insideradar(centerpos, res/2.15f, o)){
		vec ret(o.x, o.y, 0);
		ret.sub(centerpos).normalize().mul(res/2.15f).add(centerpos);
		return ret;
	}
	return insideradar(centerpos, res/2, o) ? o : vec(-1, -1, -1);
}

bool isattacking(playerent *p) { return lastmillis-p->lastaction < 500; }

vec getradarpos()
{
	float radarviewsize = VIRTH/6;
	float overlaysize = radarviewsize*4.0f/3.25f;
	return vec(VIRTW-10-VIRTH/28-overlaysize, 10+VIRTH/52, 0);
}

VARP(radarenemyfade, 0, 1250, 1250);

// DrawCircle from http://slabode.exofire.net/circle_draw.shtmls (public domain, but I'll reference it anyways
#define circleSegments 720
void DrawCircle(float cx, float cy, float r, float coordmul, int *col, float thickness = 1.f, float opacity = 250 / 255.f){
	float theta = 2 * 3.1415926 / float(circleSegments); 
	float c = cosf(theta); // precalculate the sine and cosine
	float s = sinf(theta);
	float t;

	float x = r * coordmul;// we start at angle = 0 
	float y = 0; 
    
	glEnable(GL_BLEND);
	glEnable(GL_LINE_SMOOTH);
	glLineWidth(thickness);
	glBegin(GL_LINE_LOOP);
	glColor4f(col[0] / 255.f, col[1] / 255.f, col[2] / 255.f, opacity);
	for(int ii = 0; ii < circleSegments; ii++){
		glVertex2f(x + cx * coordmul, y + cy * coordmul); // output vertex
		// apply the rotation matrix
		t = x;
		x = c * x - s * y;
		y = s * t + c * y;
	}
	glEnd(); 
	glDisable(GL_BLEND);
	glDisable(GL_LINE_SMOOTH);
}

float easedradarsize = 64;
void drawradar(playerent *p, int w, int h)
{
	vec center = showmap ? vec(ssize/2, ssize/2, 0) : p->o;
	easedradarsize = clamp(((easedradarsize * 60.f + p->o.dist(worldpos) * 3.f) / 61.f), 100.f, ssize/2.f); // 2.5f is normal scaling; 3 for extra view
	float res = showmap ? ssize : easedradarsize;

	float worldsize = (float)ssize;
	float radarviewsize = showmap ? VIRTH : VIRTH/6;
	float radarsize = worldsize/res*radarviewsize;
	float iconsize = radarentsize/res*radarviewsize;
	float coordtrans = radarsize/worldsize;
	float overlaysize = radarviewsize*4.0f/3.25f;

	glColor3f(1.0f, 1.0f, 1.0f);
	glPushMatrix();

	if(showmap) glTranslatef(VIRTW/2-radarviewsize/2, 0, 0);
	else
	{
		glTranslatef(VIRTW-VIRTH/28-radarviewsize-(overlaysize-radarviewsize)/2-10+radarviewsize/2, 10+VIRTH/52+(overlaysize-radarviewsize)/2+radarviewsize/2, 0);
		glRotatef(-camera1->yaw, 0, 0, 1);
		glTranslatef(-radarviewsize/2, -radarviewsize/2, 0);
	}

	extern GLuint minimaptex;

	vec centerpos(min(max(center.x, res/2.0f), worldsize-res/2.0f), min(max(center.y, res/2.0f), worldsize-res/2), 0.0f);
	if(showmap)
	{
		glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_COLOR);
		quad(minimaptex, 0, 0, radarviewsize, (centerpos.x-res/2)/worldsize, (centerpos.y-res/2)/worldsize, res/worldsize);
		glDisable(GL_BLEND);
	}
	else
	{
		glDisable(GL_BLEND);
		circle(minimaptex, radarviewsize/2, radarviewsize/2, radarviewsize/2, centerpos.x/worldsize, centerpos.y/worldsize, res/2/worldsize);
	}
	glTranslatef(-(centerpos.x-res/2)/worldsize*radarsize, -(centerpos.y-res/2)/worldsize*radarsize, 0);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	drawradarent(fixradarpos(p->o, centerpos, res), coordtrans, p->yaw, p->state!=CS_DEAD ? (isattacking(p) ? 2 : 0) : 1, 2, iconsize, isattacking(p) ? 1 : 0, p->state==CS_DEAD ? .5f : 1, "\f1%s", colorname(p)); // local player

	// radar check
	const bool hasradar = radarup(p);

	loopv(players) // other players
	{
		playerent *pl = players[i];
		if(!pl || pl == p) continue;
		bool force =
			hasradar || // radar earned
			p->team == TEAM_SPECT || // spectating?
			pl->state == CS_DEAD || // dead player
			(flaginfos[0].state == CTFF_STOLEN && pl == flaginfos[0].actor) || // CTF flag holders
			(flaginfos[1].state == CTFF_STOLEN && pl == flaginfos[1].actor) ||
			isteam(p, pl); // same team
		if(force)
			drawradarent(fixradarpos(pl->o, centerpos, res, pl->state==CS_DEAD), coordtrans, pl->yaw, pl->state == CS_DEAD ? 1 : isattacking(pl) ? 2 : 0,
				pl->team == TEAM_SPECT ? 0 : isteam(p, pl) ? 1 : (p->team == TEAM_SPECT) ? (pl->team * 2) : 0, iconsize, isattacking(pl) ? 1 : 0, pl->team == TEAM_SPECT ? .2f : pl->state==CS_DEAD ? .5f : 1, "\f%d%s", pl->team == TEAM_SPECT ? 4 : isteam(p, pl) ? 0 : (p->team == TEAM_SPECT) ? team_color(pl->team) : 3, colorname(pl));
		else
		{
			int taggedmillis = 0;
			extern bool IsVisible(vec v1, vec v2, dynent *tracer = NULL, bool SkipTags=false);
			if(pl->perk1 != PERK_NINJA){
				if(p->perk2 == PERK_RADAR && IsVisible(p->o, pl->o)) taggedmillis = 750;
				else loopvj(players){
					playerent *pll = players[j];
					if(!pll || p == pll || !isteam(p, pll) || (pll->state != CS_ALIVE && pll->state != CS_EDITING) || pll->perk2 != PERK_RADAR) continue;
					if(IsVisible(pll->o, pl->o)) { taggedmillis = 600; break; }
				}
			}
			if(taggedmillis){
				pl->radarmillis = lastmillis + taggedmillis;
				pl->lastloudpos[0] = pl->o.x;
				pl->lastloudpos[1] = pl->o.y;
				pl->lastloudpos[2] = pl->yaw;
			}
			if(pl->radarmillis + radarenemyfade >= lastmillis)
				drawradarent(fixradarpos(pl->lastloudpos, centerpos, res, pl->state==CS_DEAD), coordtrans, pl->lastloudpos[2], /*pl->state == CS_DEAD ? 1 :*/ isattacking(pl) ? 2 : 0,
					isteam(p, pl) ? 1 : 0, iconsize, 0, (radarenemyfade - lastmillis + pl->radarmillis) / (float)radarenemyfade, "\f3%s", colorname(pl));
		}
	}
	loopv(bounceents){ // draw grenades
		bounceent *b = bounceents[i];
		if(!b || b->bouncetype != BT_NADE) continue;
		if(((grenadeent *)b)->nadestate != 1) continue;
		drawradarent(fixradarpos(vec(b->o.x, b->o.y, 0), centerpos, res), coordtrans, 0, b->owner == p ? 2 : isteam(b->owner, p) ? 1 : 0, 3, iconsize/1.5f, 1);
	}
	int col[3] = {255, 255, 255};
	#define setcol(c1, c2, c3) col[0] = c1; col[1] = c2; col[2] = c3;
	glDisable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	loopv(nxp){ // draw explosions
		int ndelay = lastmillis - nxp[i].millis;
		if(ndelay > 600) nxp.remove(i--);
		else{
			if(nxp[i].owner == p) {setcol(0xf7, 0xf5, 0x34)} // yellow for your explosions
			else if(isteam(p, nxp[i].owner)) {setcol(0x02, 0x13, 0xFB)} // blue for friendlies' explosions
			else {setcol(0xFB, 0x02, 0x02)} // red for enemies' explosions
			vec nxpo(nxp[i].o[0], nxp[i].o[1], 0);
			nxpo = fixradarpos(nxpo, centerpos, res);
			if(ndelay / 400.f < 1) DrawCircle(nxpo.x, nxpo.y, ndelay / 100.f, coordtrans, col, 2.f, 1 - ndelay / 400.f);
			DrawCircle(nxpo.x, nxpo.y, pow(ndelay, 1.5f) / 3094.0923f, coordtrans, col, 1.f, 1 - ndelay / 600.f);
		}
	}
	loopv(sls){ // shotlines
		if(sls[i].expire < lastmillis) sls.remove(i--);
		else{
			if(sls[i].owner == p) {setcol(0x94, 0xB0, 0xDE)} // blue for your shots
			else if(isteam(p, sls[i].owner)) {setcol(0xB8, 0xDC, 0x78)} // light green-yellow for friendlies
			else {setcol(0xFF, 0xFF, 0xFF)} // white for enemies
			glBegin(GL_LINES);
			vec from(sls[i].from[0], sls[i].from[1], 0), to(sls[i].to[0], sls[i].to[1], 0);
			from = fixradarpos(from, centerpos, res);
			to = fixradarpos(to, centerpos, res);
			// source shot
			glColor4ub(col[0], col[1], col[2], 200);
			glVertex2f(from.x*coordtrans, from.y*coordtrans);
			// dest shot
			glColor4ub(col[0], col[1], col[2], 250);
			glVertex2f(to.x*coordtrans, to.y*coordtrans);
			glEnd();
		}
	}
	glEnable(GL_TEXTURE_2D);

	const float flag_yaw = showmap ? 0 : camera1->yaw;
	if(m_affinity(gamemode))
	{
		glColor4f(1.0f, 1.0f, 1.0f, (sinf(lastmillis / 100.0f) + 1.0f) / 2.0f);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		if(m_secure(gamemode))
		{
			loopv(ents)
			{
				entity &e = ents[i];
				if(e.type == CTF_FLAG && e.attr2 >= 2 && !OUTBORD(ents[i].x, ents[i].y))
				{
					float overthrown = ents[i].attr4 / 2.55f, owned = 100.f - overthrown;
					vec pos1(e.x, e.y, S(ents[i].x, ents[i].y)->floor); // location (for base)
					vec pos(0.5f-0.1f, 0.5f-0.9f, 0); // location of flag
					pos.mul(iconsize/coordtrans).rotate_around_z(flag_yaw*RAD).add(pos1);
					const int newteam = (ents[i].attr2 == TEAM_SPECT + 2 || m_gsp1(gamemode, mutators)) ? ents[i].attr3 : TEAM_SPECT;
					// t,3: bases, 3,t: flags
					drawradarent(fixradarpos(pos, centerpos, res), coordtrans, flag_yaw, clamp<int>(ents[i].attr2 - 2, TEAM_RED, TEAM_SPECT), 3, iconsize, 0, owned / 100);
					drawradarent(fixradarpos(pos, centerpos, res), coordtrans, flag_yaw, newteam, 3, iconsize, 0, overthrown / 100);
					//drawradarent(fixradarpos(pos1, centerpos, res), coordtrans, flag_yaw, 2, 3, iconsize, 0, .4f, overthrown ? "\f%d%d%%\f4/\f%d%d%%" : "", team_color(ents[i].attr2 - 2), (int)owned, team_color(newteam), (int)overthrown); // draw secure bases
				}
			}
		}
		else
		{
			loopi(2) // flag items
			{
				flaginfo &f = flaginfos[i];
				entity *e = f.flagent;
				if(!e || OUTBORD(e->x, e->y)) continue;
				vec pos1(e->x, e->y, S(e->x, e->y)->floor);
				drawradarent(fixradarpos(pos1, centerpos, res), coordtrans, flag_yaw, m_keep(gamemode) && !m_ktf2(gamemode, mutators) && f.state!=CTFF_IDLE ? 2 : f.team, 3, iconsize); // draw bases
				vec pos(0.5f-0.1f, 0.5f-0.9f, 0);
				pos.mul(iconsize/coordtrans).rotate_around_z(flag_yaw*RAD);
				if(f.state==CTFF_STOLEN && f.actor){
					pos.add(f.actor->o);
					// see flag position no matter what!
					drawradarent(fixradarpos(pos, centerpos, res), coordtrans, flag_yaw, 3, m_keep(gamemode) && !m_ktf2(gamemode, mutators) ? 2 : f.team, iconsize, f.team+1); // draw near flag thief
				}
				else{
					if(f.state == CTFF_DROPPED){
						pos.x += f.pos.x;
						pos.y += f.pos.y;
						pos.z += f.pos.z;
					}
					else pos.add(pos1);
				
					drawradarent(fixradarpos(pos, centerpos, res), coordtrans, flag_yaw, 3, m_keep(gamemode) && !m_ktf2(gamemode, mutators) && f.state != CTFF_IDLE ? 2 : f.team, iconsize, 0, f.state == CTFF_IDLE ? .3f : 1);
				}
			}
		}
	}
	else if(m_edit(gamemode))
	{
		loopv(ents) if(ents[i].type == CTF_FLAG && !OUTBORD(ents[i].x, ents[i].y)) drawradarent(fixradarpos(vec(ents[i].x, ents[i].y, S(ents[i].x, ents[i].y)->floor), centerpos, res), coordtrans, flag_yaw, clamp<int>(ents[i].attr2, 0, 2), 3, iconsize);
	}

	glEnable(GL_BLEND);
	glPopMatrix();

	if(!showmap){
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glColor3f(1, 1, 1);
		static Texture *bordertex = textureload("packages/misc/compass-base.png", 3);
		quad(bordertex->id, VIRTW-10-VIRTH/28-overlaysize, 10+VIRTH/52, overlaysize, 0, 0, 1, 1);
		if(show_hud_element(!hidecompass, 5)){
			static Texture *compasstex = textureload("packages/misc/compass-rose.png", 3);
			glPushMatrix();
			glTranslatef(VIRTW-10-VIRTH/28-overlaysize/2, 10+VIRTH/52+overlaysize/2, 0);
			glRotatef(-camera1->yaw, 0, 0, 1);
			quad(compasstex->id, -overlaysize/2, -overlaysize/2, overlaysize, 0, 0, 1, 1);
			glPopMatrix();
		}
	}
}

void drawteamicons(int w, int h){
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	const float bteam = player1->team == TEAM_BLUE ? 1 : 0;
	const float rteam = player1->team == TEAM_RED ? 1 : 0;
	static Texture *icons = textureload("packages/misc/teamicons.png", 3);
	if(rteam){
		glColor4f(1, 1, 1, rteam);
		quad(icons->id, VIRTW-VIRTH/12-10, 10, VIRTH/12, 0.f, 0, 0.49f, 1.0f);
	}
	if(bteam){
		glColor4f(1, 1, 1, bteam);
		quad(icons->id, VIRTW-VIRTH/12-10, 10, VIRTH/12, .5f, 0, 0.49f, 1.0f);
	}
}

int damageblendmillis = 0; // non-regen only

VARFP(damagescreen, 0, 1, 1, { if(!damagescreen) damageblendmillis = 0; });
VARP(damagescreenalpha, 45, 50, 100);
VARP(damagescreenfactor, 1, 4, 100); // non-regen only
VARP(damagescreenfade, 0, 325, 1000); // non-regen only
VARP(damageindicatorfade, 0, 2000, 10000);
VARP(damageindicatorsize, 0, 200, 10000);
VARP(damageindicatordist, 0, 500, 10000);

void damageblend(int n)
{
	if(!damagescreen || m_regen(gamemode, mutators)) return;
	if(n <= 0)
	{
		damageblendmillis = 0;
	}
	else
	{
		if(lastmillis > damageblendmillis) damageblendmillis = lastmillis;
		damageblendmillis += min(n*damagescreenfactor, 5000);
	}
}

VARP(hitmarkerfade, 1, 750, 5000);

static int votersort(playerent **a, playerent **b){
	return (*a)->voternum - (*b)->voternum;
}

void gl_drawhud(int w, int h, int curfps, int nquads, int curvert, bool underwater){
	bool spectating = player1->isspectating();

	glDisable(GL_DEPTH_TEST);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, VIRTW, VIRTH, 0, -1, 1);
	glEnable(GL_BLEND);

	if(underwater)
	{
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glColor4ub(hdr.watercolor[0], hdr.watercolor[1], hdr.watercolor[2], 102);

		glBegin(GL_QUADS);
		glVertex2f(0, 0);
		glVertex2f(VIRTW, 0);
		glVertex2f(VIRTW, VIRTH);
		glVertex2f(0, VIRTH);
		glEnd();
	}

	glDisable(GL_TEXTURE_2D);

	/*
	if(focus->flashmillis > 0 && lastmillis<=focus->flashmillis){
		extern GLuint flashtex;
		if(flashtex){
			glEnable(GL_TEXTURE_2D);
			glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
			glBindTexture(GL_TEXTURE_2D, flashtex);

			const float flashsnapfade = min((focus->flashmillis - lastmillis) / 1500.f, .78f);
			glColor4f(flashsnapfade, flashsnapfade, flashsnapfade, flashsnapfade);

			glBegin(GL_QUADS);
			glTexCoord2f(0, 0); glVertex2f(0, 0);
			glTexCoord2f(1, 0); glVertex2f(VIRTW, 0);
			glTexCoord2f(1, 1); glVertex2f(VIRTW, VIRTH);
			glTexCoord2f(0, 1); glVertex2f(0, VIRTH);
			glEnd();
		}

		// flashbang!
		glDisable(GL_TEXTURE_2D);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		const float flashwhitefade = min((focus->flashmillis - lastmillis - 1500) / 1500.f, .6f);
		glColor4f(1, 1, 1, flashwhitefade);
		
		glBegin(GL_QUADS);
		glVertex2f(0, 0);
		glVertex2f(VIRTW, 0);
		glVertex2f(VIRTW, VIRTH);
		glVertex2f(0, VIRTH);
		glEnd();
	}
	*/

	static Texture *damagetex = textureload("packages/misc/damage.png", 3), *damagedirtex = textureload("packages/misc/damagedir.png");
	glEnable(GL_TEXTURE_2D);

	if(damagescreen){
		static float fade = 0.f;
		if(m_regen(gamemode, mutators)){
			const int maxhealth = 100 * HEALTHSCALE;
			float newfade = 0;
			if(focus->state == CS_ALIVE && focus->health >= 0 && focus->health < maxhealth)
				newfade = sqrtf(1.f - focus->health / (float)maxhealth);
			fade = clamp((fade * 40 + newfade) / 41.f, 0.f, 1.f);
		}
		else if(lastmillis < damageblendmillis)
		{
			fade = 1.f;
			if(damageblendmillis - lastmillis < damagescreenfade)
				fade *= float(damageblendmillis - lastmillis)/damagescreenfade;
		}
		else fade = 0;
		if(fade >= .05f){
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			glBindTexture(GL_TEXTURE_2D, damagetex->id);
			const float c = clamp(fade, .05f, .95f);
			glColor4f(1, 1, 1, c * damagescreenalpha / 100.f);

			glBegin(GL_QUADS);
			glTexCoord2f(0, 0); glVertex2f(0, 0);
			glTexCoord2f(1, 0); glVertex2f(VIRTW, 0);
			glTexCoord2f(1, 1); glVertex2f(VIRTW, VIRTH);
			glTexCoord2f(0, 1); glVertex2f(0, VIRTH);
			glEnd();
		}
	}

	loopv(focus->damagestack){
		damageinfo &pain = focus->damagestack[i];
		const float damagefade = damageindicatorfade + pain.damage*20;
		if(pain.millis + damagefade <= lastmillis){ focus->damagestack.remove(i--); continue; }
		vec dir = pain.o;
		if(dir == focus->o) continue;
		dir.sub(focus->o).normalize();
		const float fade = 1 - (lastmillis-pain.millis)/damagefade, size = damageindicatorsize, dirangle = dir.x ? atan2f(dir.y, dir.x) / RAD : dir.y < 0 ? 270 : 90;
		glPushMatrix();
		glTranslatef(VIRTW/2, VIRTH/2, 0);
		glRotatef(dirangle + 90 - player1->yaw, 0, 0, 1);
		glTranslatef(0, -damageindicatordist, 0);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glBindTexture(GL_TEXTURE_2D, damagedirtex->id);
		glColor4f(fade * 2, fade * 2, fade * 2, fade * 2);

		glBegin(GL_QUADS);
		glTexCoord2f(0, 0); glVertex2f(-size, -size / 2);
		glTexCoord2f(1, 0); glVertex2f(size, -size / 2);
		glTexCoord2f(1, 1); glVertex2f(size, size / 2);
		glTexCoord2f(0, 1); glVertex2f(-size, size / 2);
		glEnd();
		glPopMatrix();
	}

	bool menu = menuvisible();
	bool command = getcurcommand() ? true : false;

	// hitmarker
	if(show_hud_element(true, 2) && focus->lasthitmarker && focus->lasthitmarker + hitmarkerfade > lastmillis){
		glColor4f(1, 1, 1, (focus->lasthitmarker + hitmarkerfade - lastmillis) / 1000.f);
		Texture *ch = crosshairs[CROSSHAIR_HIT];
		if(!ch) ch = textureload("packages/misc/crosshairs/hit.png", 3);
		if(ch->bpp==32) glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		else glBlendFunc(GL_SRC_ALPHA, GL_ONE);

		glBindTexture(GL_TEXTURE_2D, ch->id);
		glBegin(GL_QUADS);
		const float hitsize = 56.f;
		glTexCoord2f(0, 0); glVertex2f(VIRTW/2 - hitsize, VIRTH/2 - hitsize);
		glTexCoord2f(1, 0); glVertex2f(VIRTW/2 + hitsize, VIRTH/2 - hitsize);
		glTexCoord2f(1, 1); glVertex2f(VIRTW/2 + hitsize, VIRTH/2 + hitsize);
		glTexCoord2f(0, 1); glVertex2f(VIRTW/2 - hitsize, VIRTH/2 + hitsize);
		glEnd();
	}

	// crosshair
	if(!focus->weaponsel->reloading && !focus->weaponchanging){
		if(focus->state==CS_EDITING) drawcrosshair(focus, CROSSHAIR_SCOPE, worldhit && worldhit->state==CS_ALIVE ? isteam(worldhit, focus) ? 1 : 2 : 0, NULL, 48.f);
		else if(focus->state!=CS_DEAD && (!m_zombie(gamemode) || focus->thirdperson >= 0)) focus->weaponsel->renderaimhelp(worldhit && worldhit->state==CS_ALIVE ? isteam(worldhit, focus) ? 1 : 2 : 0);
	}

	// fake red-dot
	if(focus->ads >= crosshairreddottreshold){ // show red-dot when 750 zoomed
		glColor4f(1, 1, 1, powf(focus->ads / 1000.f, 2.f));
		Texture *ch = crosshairs[CROSSHAIR_REDDOT];
		if(!ch) ch = textureload("packages/misc/crosshairs/reddot.png", 3);
		if(ch->bpp==32) glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		else glBlendFunc(GL_SRC_ALPHA, GL_ONE);

		glBindTexture(GL_TEXTURE_2D, ch->id);
		glBegin(GL_QUADS);
		const float hitsize = crosshairreddotsize * 2; // fixme
		glTexCoord2f(0, 0); glVertex2f(VIRTW/2 - hitsize, VIRTH/2 - hitsize);
		glTexCoord2f(1, 0); glVertex2f(VIRTW/2 + hitsize, VIRTH/2 - hitsize);
		glTexCoord2f(1, 1); glVertex2f(VIRTW/2 + hitsize, VIRTH/2 + hitsize);
		glTexCoord2f(0, 1); glVertex2f(VIRTW/2 - hitsize, VIRTH/2 + hitsize);
		glEnd();
	}

	static Texture **texs = geteventicons();
	if(!isthirdperson) loopv(focus->icons){
		eventicon &icon = focus->icons[i];
		if(icon.type < 0 || icon.type >= eventicon::TOTAL){
			focus->icons.remove(i--);
			continue;
		}
		if(icon.millis + 3000 < lastmillis) continue; // deleted elsewhere
		Texture *tex = texs[icon.type];
		int h = 1;
		float aspect = 1, scalef = 1, offset = (lastmillis - icon.millis) / 3000.f * 160.f;
		switch(icon.type){
			case eventicon::CHAT:
			case eventicon::VOICECOM:
			case eventicon::PICKUP:
				scalef = .4f;
				break;
			case eventicon::HEADSHOT:
			case eventicon::CRITICAL:
			case eventicon::REVENGE:
			case eventicon::FIRSTBLOOD:
				aspect = 2;
				h = 4;
				break;
			case eventicon::DECAPITATED:
			case eventicon::BLEED:
				scalef = .4f;
				break;
			default:
				scalef = .3f;
				break;
		}
		glBindTexture(GL_TEXTURE_2D, tex->id);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glEnable(GL_BLEND);
		glColor4f(1.f, 1.f, 1.f, (3000 + icon.millis - lastmillis) / 3000.f);
		glBegin(GL_QUADS);
		float anim = lastmillis / 100 % (h * 2);
		if(anim >= h) anim = h * 2 - anim + 1;
		anim /= h;
		const float xx = VIRTW * .15f * scalef, yy = /*VIRTH * .2f * scalef*/ xx / aspect, yoffset = VIRTH * -.15f - offset;
		glTexCoord2f(0, anim); glVertex2f(VIRTW / 2 - xx, VIRTH / 2 - yy + yoffset);
		glTexCoord2f(1, anim); glVertex2f(VIRTW / 2 + xx, VIRTH / 2 - yy + yoffset);
		anim += 1.f / h;
		glTexCoord2f(1, anim); glVertex2f(VIRTW / 2 + xx, VIRTH / 2 + yy + yoffset);
		glTexCoord2f(0,	anim); glVertex2f(VIRTW / 2 - xx, VIRTH / 2 + yy + yoffset);
		glEnd();
	}

	drawequipicons(focus);

	glMatrixMode(GL_MODELVIEW);
	if(/*!menu &&*/ (show_hud_element(!hideradar, 5) || showmap)) drawradar(focus, w, h);
	if(show_hud_element(!hideteam, 1) && m_team(gamemode, mutators)) drawteamicons(w, h);
	glMatrixMode(GL_PROJECTION);

	char *infostr = editinfo();
	int commandh = 1570 + FONTH;
	if(command) commandh -= rendercommand(20, 1570, VIRTW);
	else if(infostr) draw_text(infostr, 20, 1570);
	else if(show_hud_element(true, 1)){
		defformatstring(hudtext)("\f0[\f1%04.1f\f3m\f0]", focus->o.dist(worldhitpos) / 4.f);
		static string hudtarget;
		static int lasttarget = INT_MIN;
		if(worldhit){
			formatstring(hudtarget)(" \f2[\f%d%s\f2] \f4[\f%s\f4]", team_rel_color(focus, worldhit), colorname(worldhit),
				worldhitzone==HIT_HEAD?"3HEAD":worldhitzone==HIT_TORSO?"2TORSO":"0LEGS");
			concatstring(hudtext, hudtarget);
			lasttarget = lastmillis;
		}
		else if(lastmillis - lasttarget < 800){
			const short a = (800 - lastmillis + lasttarget) * 255 / 800;
			draw_text(hudtarget, 20 + text_width(hudtext), 1570, a, a, a, a);
		}
		draw_text(hudtext, 20, 1570);
	}

	extern int lastexpadd, lastexptexttime;
	if(lastmillis <= lastexpadd + COMBOTIME){
		extern int lastexpaddamt;
		defformatstring(scoreaddtxt)("\f%c%+d", !lastexpaddamt ? '4' : lastexpaddamt >= 0 ? '2' : '3', lastexpaddamt);
		const short a = (lastexpadd + COMBOTIME - lastmillis) * 255 / COMBOTIME;
		draw_text(scoreaddtxt, VIRTW*11/20, VIRTH*8/20, a, a, a, a);
	}

	if(lastmillis <= lastexptexttime + COMBOTIME){
		extern string lastexptext;
		const short a = (lastexptexttime + COMBOTIME - lastmillis) * 255 / COMBOTIME;
		draw_text(lastexptext, VIRTW*11/20, VIRTH*8/20 + FONTH, a, a, a, a);
	}

	glLoadIdentity();
	glOrtho(0, VIRTW*2, VIRTH*2, 0, -1, 1);

	if(!hideconsole) renderconsole();
	if(show_hud_element(!hideobits, 4)) renderobits();
	if(!hidestats)
	{
		const int left = (VIRTW-225-10)*2, top = (VIRTH*7/8)*2;
		// semi-debug info
		draw_textf("sp2 %04.3f", left, top-160, focus->vel.magnitudexy());
		draw_textf("spd %04.3f", left, top-80, focus->vel.magnitude());
		// real info
		draw_textf("fps %d", left, top, curfps);
		draw_textf("lod %d", left, top+80, lod_factor());
		draw_textf("wqd %d", left, top+160, nquads);
		draw_textf("wvt %d", left, top+240, curvert);
		draw_textf("evt %d", left, top+320, xtraverts);
	}
	else if(hidestats == 1) draw_textf("fps %d", (VIRTW-225-10)*2, VIRTH*2-100, curfps);

	if(!intermission && !m_edit(gamemode)){
		extern int gametimecurrent, lastgametimeupdate, gametimemaximum;
		int cssec = (gametimecurrent+(lastmillis-lastgametimeupdate))/1000;
		int cursec = cssec%60;
		int curmin = cssec/60;
		
		int rmin = gametimemaximum/60000 - curmin, rsec = cursec;
		if(rsec){
			rmin--;
			rsec = 60 - rsec;
		}
		
		defformatstring(gtime)("%02d:%02d/%02d:%02d", curmin, cursec, rmin, rsec);
		draw_text(gtime, (2*VIRTW - text_width(gtime))/2, 2);
	}

	if(hidevote < 2)
	{
		extern votedisplayinfo *curvote;

		if(curvote && curvote->millis >= totalmillis && !(hidevote == 1 && player1->vote != VOTE_YES && curvote->result == VOTE_NEUTRAL))
		{
			int left = 20*2, top = VIRTH + 22*10;
			if(curvote->result == VOTE_NEUTRAL)
			draw_textf("%s called a vote: %.2f seconds remaining", left, top+240, curvote->owner ? colorname(curvote->owner) : "(unknown owner)", (curvote->expiremillis-lastmillis)/1000.0f);
			else draw_textf("%s called a vote:", left, top+240, curvote->owner ? colorname(curvote->owner) : "(unknown)");
			draw_textf("%s", left, top+320, curvote->desc);
			draw_textf("----", left, top+400);

			vector<playerent *> votepl[VOTE_NUM];
			string votestr[VOTE_NUM];
			if(!watchingdemo) votepl[player1->vote].add(player1);
			loopv(players){
				playerent *vpl = players[i];
				if(!vpl || vpl->ownernum >= 0) continue;
				votepl[vpl->vote].add(vpl);
			}
			loopl(VOTE_NUM){
				copystring(votestr[l], "");
				if(!votepl[l].length()) continue;
				// special case: hide if too many are neutral
				if(l == VOTE_NEUTRAL && votepl[VOTE_NEUTRAL].length() > 5) continue;
				votepl[l].sort(votersort);
				loopv(votepl[l]){
					playerent *vpl = votepl[l][i];
					if(!vpl) continue;
					concatformatstring(votestr[l], "\f%d%s \f6(%d)", vpl->priv ? 0 : vpl == player1 ? 6 : team_color(vpl->team), vpl->name, vpl->clientnum);
					if(vpl->priv >= PRIV_ADMIN) concatstring(votestr[l], " \f8(!)");
					concatstring(votestr[l], "\f5, ");
				}
				// trim off last space, comma, 5, and line feed
				votestr[l][strlen(votestr[l]) - 4] = '\0';
				//copystring(votestr[l], votestr[l], strlen(votestr[l])-1);
			}
			draw_textf("\fs\f%c%d yes\fr vs. \fs\f%c%d no\fr", left, top+480,
				curvote->expiryresult == VOTE_YES ? '0' : '5',
				votepl[VOTE_YES].length(),
				curvote->expiryresult == VOTE_NO ? '3' : '5',
				votepl[VOTE_NO].length());

			glBlendFunc(GL_SRC_ALPHA, GL_ONE);
			glColor4f(1.0f, 1.0f, 1.0f, (sinf(lastmillis/100.0f)+1.0f) / 2.0f);
			switch(curvote->result)
			{
				case VOTE_NEUTRAL:
					drawvoteicon(left, top, 0, 0, true);
					if(player1->vote == VOTE_NEUTRAL)
						draw_textf("\f3please vote yes or no (F1/F2)", left, top+560);
					else draw_textf("\f2you voted \f%s \f1(F%d to change)", left, top+560, player1->vote == VOTE_NO ? "3no" : "0yes", player1->vote == VOTE_NO ? 1 : 2);
					break;
				default:
					drawvoteicon(left, top, (curvote->result-1)&1, 1, false);
					draw_textf("\f%s \f%s", left, top+560, veto ? "1VETO" : "2vote", curvote->result == VOTE_YES ? "0PASSED" : "3FAILED");
					break;
			}
			glLoadIdentity();
			glOrtho(0, VIRTW*2.2, VIRTH*2.2, 0, -1, 1);
			left *= 1.1; top += 560; top *= 1.1;
			if(*votestr[VOTE_YES]){
				draw_textf("\f1Vote \f0Yes \f5(\f4%d/%d\f5)", left, top += 88, votepl[VOTE_YES].length(), curvote->yes_remain);
				draw_text(votestr[VOTE_YES], left, top += 88);
			}
			if(*votestr[VOTE_NO]){
				draw_textf("\f1Vote \f3No \f5(\f4%d/%d\f5)", left, top += 88, votepl[VOTE_NO].length(), curvote->no_remain);
				draw_text(votestr[VOTE_NO], left, top += 88);
			}
			if(*votestr[VOTE_NEUTRAL]){
				draw_textf("\f1Vote \f2Neutral \f5(\f4%d\f5)", left, top += 88, votepl[VOTE_NEUTRAL].length());
				draw_text(votestr[VOTE_NEUTRAL], left, top += 88);
			}
		}
	}

	if(menu) rendermenu();
	else if(command) renderdoc(40, VIRTH, max(commandh*2 - VIRTH, 0));

	if(!hidehudmsgs) hudmsgs.render();


	if(!hidespecthud && focus->state==CS_DEAD && focus->spectatemode<=SM_DEATHCAM)
	{
		glLoadIdentity();
		glOrtho(0, VIRTW*3/2, VIRTH*3/2, 0, -1, 1);
		const int left = (VIRTW*3/2)*6/8, top = (VIRTH*3/2)*3/4;
		draw_textf("SPACE to change view", left, top);
		draw_textf("SCROLL to change player", left, top+80);
	}

	/*
	glLoadIdentity();
	glOrtho(0, VIRTW*3/2, VIRTH*3/2, 0, -1, 1);
	const int left = (VIRTW*3/2)*4/8, top = (VIRTH*3/2)*3/4;
	draw_textf("!TEST BUILD!", left, top);
	*/

	if(!hidespecthud && spectating && player1->spectatemode!=SM_DEATHCAM)
	{
		glLoadIdentity();
		glOrtho(0, VIRTW, VIRTH, 0, -1, 1);
		draw_text(player1->spectatemode==SM_FOLLOWSAME ? "FOLLOWING" : "SPECTATING", VIRTW/40, VIRTH/10*7);
		if(player1->spectatemode==SM_FOLLOWSAME || player1->spectatemode==SM_FOLLOWALT)
		{
			if(players.inrange(player1->followplayercn) && players[player1->followplayercn])
			{
				defformatstring(name)("Player \f%d%s", team_color(players[player1->followplayercn]->team), colorname(players[player1->followplayercn]));
				draw_text(name, VIRTW/40, VIRTH/10*8);
			}
		}
	}

	//if(focus->state==CS_ALIVE)
	//{
		glLoadIdentity();
		glOrtho(0, VIRTW/2, VIRTH/2, 0, -1, 1);

		if(show_hud_element(!hidehudequipment, 3) && focus->state != CS_DEAD && focus->state != CS_EDITING)
		{
			pushfont("huddigits");
			if(show_hud_element(!hidehudequipment, 1)){
				defformatstring(healthstr)("%d", focus->health / HEALTHSCALE);
				draw_text(healthstr, 90, 823);
				if(focus->armor){
					int offset = text_width(healthstr);
					glPushMatrix();
					glScalef(0.5f, 0.5f, 1.0f);
					draw_textf("%d", (90 + offset)*2, 826*2, (focus->health / HEALTHSCALE) + focus->armor * 3 / 10);
					glPopMatrix();
				}
				//if(focus->armor) draw_textf("%d", 360, 823, focus->armor);
				//if(focus->weapons[WEAP_GRENADE] && focus->weapons[WEAP_GRENADE]->mag) focus->weapons[WEAP_GRENADE]->renderstats();
			}
			// The next set will alter the matrix - load the identity matrix and apply ortho after
			if(focus->weaponsel && focus->weaponsel->type>=WEAP_KNIFE && focus->weaponsel->type<WEAP_MAX){
				if(focus->weaponsel->type != WEAP_GRENADE) focus->weaponsel->renderstats();
				else if(focus->prevweaponsel && focus->prevweaponsel->type != WEAP_GRENADE) focus->prevweaponsel->renderstats();
				else if(focus->nextweaponsel && focus->nextweaponsel->type != WEAP_GRENADE) focus->nextweaponsel->renderstats();
			}
			popfont();
		}

		if(m_affinity(gamemode) && !m_secure(gamemode) && !hidectfhud)
		{
			glLoadIdentity();
			glOrtho(0, VIRTW, VIRTH, 0, -1, 1);
			glEnable(GL_BLEND);
			loopi(2) drawflagicons(flaginfos[i], focus); // flag state
		}
	//}

	// draw the perk icons
	
	glLoadIdentity();
	glOrtho(0, VIRTW, VIRTH, 0, -1, 1);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	if(show_hud_element(true, 1)){
		Texture *perk1 = getperktex1()[focus->perk1%PERK1_MAX], *perk2 = getperktex2()[focus->perk2%PERK2_MAX];
		if(perk1 != perk2){
			glColor4f(1.0f, 1.0f, 1.0f, focus->perk1 /* != PERK_NONE */ && focus->state != CS_DEAD ? .78f : .3f);
			quad(perk1->id, VIRTW-225-10 - 100 - 15 - 100 - 20, VIRTH - 100 - 10, 100, 0, 0, 1);
		}

		if(perk2){
			glColor4f(1.0f, 1.0f, 1.0f, focus->perk2 /* != PERK_NONE */ && focus->state != CS_DEAD ? .78f : .3f);
			quad(perk2->id, VIRTW-225-10 - 100 - 20, VIRTH - 100 - 10, 100, 0, 0, 1);
		}
	}

	// streak meter
	if(show_hud_element(showstreak, 1)){
		const float streakscale = 1.5f;
		static Texture *streakt[2][4] = { NULL };
		loopi(2) loopj(4){
			// done, current, outstanding
			defformatstring(path)("packages/misc/streak/%d%s.png", i, j ? j > 1 ? j > 2 ? "d" : "" : "c" : "o");
			streakt[i][j] = textureload(path);
		}
		glLoadIdentity();
		glOrtho(0, VIRTW * streakscale, VIRTH * streakscale, 0, -1, 1);
		// we have the blend function set by the perk icon
		const int currentstreak = floor(focus->pointstreak/5.f);
		loopi(11){
			glColor4f(1, 1, 1, focus->state != CS_DEAD ? (currentstreak == i || i >= 10) ? (0.3f+fabs(sinf(lastmillis/500.0f))/2 * ((i-1)%5)/4.f) : .8f : .3f);
			quad(streakt[i & 1][currentstreak > i ? 2 : currentstreak == i ? 1 : focus->deathstreak >= i ? 3 : 0]->id,
					(VIRTW-225-10-180-30 - 80 - 15 -(11*50) + i*50) * streakscale, (VIRTH - 80 - 35) * streakscale, 80 * streakscale, 0, 0, 1);
		}
		// streak misc
		// streak num
		if(focus->deathstreak) draw_textf("\f3-%d", (VIRTW-225-10-180-22 - 80 - 23 - max(11-focus->deathstreak,1)*50) * streakscale, (VIRTH - 50 - 40) * streakscale, focus->deathstreak);
		else draw_textf("\f%c%.1f", (VIRTW-225-10-180-22 - 80 - 23 - max(11-currentstreak,1)*50) * streakscale, (VIRTH - 50 - 40) * streakscale,
			focus->pointstreak >= 9*5 ? '1' :
			focus->pointstreak >= 7*5 ? '0' :
			focus->pointstreak >= 3*5 ? '2' :
			focus->pointstreak ? '2' :
			'4',
			focus->pointstreak/5.f);
		// airstrikes
		draw_textf("\f4x\f%c%d", (VIRTW-225-10-180-22 - 80 - 23 - 5*50) * streakscale, (VIRTH - 50) * streakscale, focus->airstrikes ? '0' : '5', focus->airstrikes);
		// radar time
		int stotal, sr;
		playerent *spl;
		radarinfo(stotal, spl, sr, focus);
		if(!sr || !spl) stotal = 0; // safety
		draw_textf("%d:\f%d%04.1f", (VIRTW-225-10-180-22 - 80 - 40 - 3*50) * streakscale, (VIRTH - 50 - 80 - 25) * streakscale, stotal, stotal ? team_rel_color(focus, spl) : 5, sr / 1000.f);
		// nuke timer
		nukeinfo(stotal, spl, sr);
		if(!sr || !spl) stotal = 0; // more safety
		draw_textf("%d:\f%d%04.1f", (VIRTW-225-10-180-22 - 80 - 40 - 50) * streakscale, (VIRTH - 50) * streakscale, stotal, stotal ? team_rel_color(focus, spl) : 5, sr / 1000.f);
	}

	// finally, we're done
	glDisable(GL_BLEND);
	glDisable(GL_TEXTURE_2D);
	glEnable(GL_DEPTH_TEST);

	glMatrixMode(GL_MODELVIEW);
}

void loadingscreen(const char *fmt, ...)
{
	static Texture *logo = textureload("packages/misc/startscreen.png", 3);

	glEnable(GL_TEXTURE_2D);
	glDisable(GL_DEPTH_TEST);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, VIRTW, VIRTH, 0, -1, 1);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glClearColor(0, 0, 0, 1);
	glColor3f(1, 1, 1);

	loopi(fmt ? 1 : 2)
	{
		glClear(GL_COLOR_BUFFER_BIT);
		quad(logo->id, (VIRTW-VIRTH)/2, 0, VIRTH, 0, 0, 1);
		if(fmt)
		{
			glEnable(GL_BLEND);
			defvformatstring(str, fmt, fmt);
			int w = text_width(str);
			draw_text(str, w>=VIRTW ? 0 : (VIRTW-w)/2, VIRTH*3/4);
			glDisable(GL_BLEND);
		}
		SDL_GL_SwapBuffers();
	}

	glDisable(GL_TEXTURE_2D);
	glEnable(GL_DEPTH_TEST);
}

static void bar(float bar, int o, float r, float g, float b)
{
	int side = 2*FONTH;
	float x1 = side, x2 = bar*(VIRTW*1.2f-2*side)+side;
	float y1 = o*FONTH;
	glColor3f(0.3f, 0.3f, 0.3f);
	glBegin(GL_TRIANGLE_STRIP);
	loopk(10)
	{
	   float c = 1.2f*cosf(M_PI/2 + k/9.0f*M_PI), s = 1 + 1.2f*sinf(M_PI/2 + k/9.0f*M_PI);
	   glVertex2f(x2 - c*FONTH, y1 + s*FONTH);
	   glVertex2f(x1 + c*FONTH, y1 + s*FONTH);
	}
	glEnd();

	glColor3f(r, g, b);
	glBegin(GL_TRIANGLE_STRIP);
	loopk(10)
	{
	   float c = cosf(M_PI/2 + k/9.0f*M_PI), s = 1 + sinf(M_PI/2 + k/9.0f*M_PI);
	   glVertex2f(x2 - c*FONTH, y1 + s*FONTH);
	   glVertex2f(x1 + c*FONTH, y1 + s*FONTH);
	}
	glEnd();
}

void show_out_of_renderloop_progress(float bar1, const char *text1, float bar2, const char *text2)   // also used during loading
{
	c2skeepalive();

	glDisable(GL_DEPTH_TEST);
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glOrtho(0, VIRTW*1.2f, VIRTH*1.2f, 0, -1, 1);

	glLineWidth(3);

	if(text1)
	{
		bar(1, 1, 0.1f, 0.1f, 0.1f);
		if(bar1>0) bar(bar1, 1, 0.2f, 0.2f, 0.2f);
	}

	if(bar2>0)
	{
		bar(1, 3, 0.1f, 0.1f, 0.1f);
		bar(bar2, 3, 0.2f, 0.2f, 0.2f);
	}

	glLineWidth(1);

	glEnable(GL_BLEND);
	glEnable(GL_TEXTURE_2D);

	if(text1) draw_text(text1, 2*FONTH, 1*FONTH + FONTH/2);
	if(bar2>0) draw_text(text2, 2*FONTH, 3*FONTH + FONTH/2);

	glDisable(GL_TEXTURE_2D);
	glDisable(GL_BLEND);

	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();

	glEnable(GL_DEPTH_TEST);
	SDL_GL_SwapBuffers();
}

void renderhudwaypoints(){
	// throwing knife pickups
	loopv(knives){
		vec s;
		bool ddt = focus->perk2 == PERK2_VISION;
		if(!ddt){
			vec dir, s;
			(dir = focus->o).sub(knives[i].o).normalize();
			ddt = rayclip(knives[i].o, dir, s) + 1.f >= focus->o.dist(knives[i].o);
		}
		renderwaypoint(WP_KNIFE, knives[i].o, (float)(knives[i].millis - totalmillis) / KNIFETTL, ddt);
	}
	// vision perk
	if(focus->perk2 == PERK2_VISION) loopv(bounceents){
		bounceent *b = bounceents[i];
		if(!b || (b->bouncetype != BT_NADE && b->bouncetype != BT_KNIFE)) continue;
		if(b->bouncetype == BT_NADE && ((grenadeent *)b)->nadestate != 1) continue;
		if(b->bouncetype == BT_KNIFE && ((knifeent *)b)->knifestate != 1) continue;
		renderwaypoint(b->bouncetype == BT_NADE ? WP_EXP : WP_KNIFE, b->o);
	}
	// flags
	const int teamfix = focus->team == TEAM_SPECT ? TEAM_RED : focus->team;
	if(m_affinity(gamemode))
	{
		if(m_secure(gamemode)) loopv(ents)
		{
			entity &e = ents[i];
			if(e.type == CTF_FLAG && e.attr2 >= 2)
			{
				vec o(e.x, e.y, (float)S(int(e.x), int(e.y))->floor + PLAYERHEIGHT);
				renderwaypoint(e.attr2 - 2 == TEAM_SPECT ? WP_SECURE : (e.attr2 - 2) == teamfix ? WP_DEFEND : WP_OVERTHROW, o, e.attr4 ? fabs(sinf(lastmillis/200.f)) : 1.f, true);
				if(e.attr4)
				{
					float progress = e.attr4 / 255.f;
					renderprogress_back(o, color(0, 0, 0, .35f));
					if(m_gsp1(gamemode, mutators))
						renderprogress(o, progress, e.attr3 ? color(0, 0, 1, .28f) : color(1, 0, 0, .28f));
					else
					{
						renderprogress(o, e.attr2 - 2 == TEAM_SPECT ? .5f : progress / 2.f, color(1, 1, 1, .28f));
						if(e.attr2 - 2 == TEAM_SPECT)
							renderprogress(o, progress / 2.f, e.attr3 ? color(0, 0, 1, .28f) : color(1, 0, 0, .28f), .5f);
					}
				}
			}
		}
		else loopi(2)
		{
			const float a = 1;
			int wp = -1;
			vec o;

			flaginfo &f = flaginfos[i];
			entity &e = *f.flagent;

			// flag
			switch(f.state)
			{
				case CTFF_STOLEN:
					if(f.actor == focus && !isthirdperson) break;
					if(OUTBORD(f.actor->o.x, f.actor->o.y)) break;
					o = f.actor->o;
					wp = (focus == f.actor || (m_team(gamemode, mutators) && f.actor->team == teamfix)) ?
						// friendly
						(m_capture(gamemode) || m_bomber(gamemode)) ? WP_ESCORT : WP_DEFEND
						: // hostile below
						WP_KILL;
					break;
				case CTFF_DROPPED:
					if(OUTBORD(f.pos.x, f.pos.y)) break;
					o = f.pos;
					o.z += PLAYERHEIGHT;
					if(m_capture(gamemode)) wp = i == teamfix ? WP_RETURN : WP_ENEMY;
					else if(m_keep(gamemode)) wp = WP_ENEMY;
					else if(m_bomber(gamemode)) wp = i == teamfix ? WP_BOMB : WP_DEFUSE;
					else wp = i == teamfix ? WP_FRIENDLY : WP_GRAB;
					break;
			}
			o.z += PLAYERABOVEEYE;
			if(wp >= 0 && wp < WP_NUM) renderwaypoint(wp, o, a);

			if(OUTBORD(e.x, e.y)) continue;

			// flag base
			wp = WP_STOLEN; // "wait"
			switch(f.state){
				default: // stolen or dropped
					if(m_bomber(gamemode)) wp = flaginfos[team_opposite(i)].state != CTFF_INBASE ? i == teamfix ? WP_DEFEND : WP_TARGET : -1;
					else if(m_keep(gamemode) ? (f.actor != focus && !isteam(f.actor, focus)) : m_team(gamemode, mutators) ? (i != teamfix) : (f.actor != focus)) wp = -1; break;
				case CTFF_INBASE:
					if(m_capture(gamemode)){
						wp = i == teamfix ? WP_FRIENDLY : WP_GRAB;
					}
					else if(m_bomber(gamemode))
						wp = i == teamfix ? WP_BOMB : WP_TARGET;
					else if(m_hunt(gamemode))
						wp = i == teamfix ? WP_FRIENDLY : WP_ENEMY;
					else{ // if(m_keep(gamemode)){
						wp = WP_GRAB;
					}
					break;
				case CTFF_IDLE: // KTF only
					// WAIT here if the opponent has the flag
					if(flaginfos[team_opposite(i)].state == CTFF_STOLEN && flaginfos[team_opposite(i)].actor && focus != flaginfos[team_opposite(i)].actor && !isteam(flaginfos[team_opposite(i)].actor, focus))
						break;
					wp = WP_ENEMY;
					break;
			}
			if(wp >= 0 && wp < WP_NUM) renderwaypoint(wp, vec(e.x, e.y, (float)S(int(e.x), int(e.y))->floor + PLAYERHEIGHT), a);
		}
	}
	loopv(players){
		playerent *pl = i == getclientnum() ? player1 : players[i];
		if(!pl || (pl == focus && !isthirdperson) || pl->state == CS_DEAD) continue;
		const bool has_flag = m_affinity(gamemode) && (flaginfos[0].state == CTFF_STOLEN && flaginfos[0].actor_cn == i) || (flaginfos[1].state == CTFF_STOLEN && flaginfos[1].actor_cn == i);
		if(has_flag) continue;
		const bool has_nuke = pl->nukemillis >= totalmillis;
		if(has_nuke || m_psychic(gamemode, mutators)){
			renderwaypoint((focus == pl || isteam(focus, pl)) ? WP_DEFEND : WP_KILL, pl->o);
			if(has_nuke) renderwaypoint(WP_NUKE, vec(pl->o.x, pl->o.y, pl->o.z + PLAYERHEIGHT));
		}
	}
}