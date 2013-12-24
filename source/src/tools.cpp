// implementation of generic tools

#include "pch.h"
#include "cube.h"

///////////////////////// file system ///////////////////////

#ifndef WIN32
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#endif

#if defined(WIN32) && defined(__GNUC__)
#define KEY_WOW64_64KEY	0x0100
#endif // WIN32 && __GNUC__

string homedir = "";
vector<char *> packagedirs;

#ifdef WIN32
char *getregszvalue(HKEY root, const char *keystr, const char *query, REGSAM extraaccess = 0)
{
	HKEY key;
	if(RegOpenKeyEx(root, keystr, 0, KEY_READ | extraaccess, &key)==ERROR_SUCCESS)
	{
		DWORD type = 0, len = 0;
		if(RegQueryValueEx(key, query, 0, &type, 0, &len)==ERROR_SUCCESS && type==REG_SZ)
		{
			char *val = new char[len];
			long result = RegQueryValueEx(key, query, 0, &type, (uchar *)val, &len);
			if(result==ERROR_SUCCESS)
			{
				RegCloseKey(key);
				val[len-1] = '\0';
				return val;
			}
			delete[] val;
		}
		RegCloseKey(key);
	}
	return NULL;
}
#endif

unsigned int &genguid(int b, uint a, int c, const char* z)
{
	static unsigned int value = 0;
	value = 0;
	unsigned int temp = 0;
	extern void *basicgen();
	char *inpStr = (char *)basicgen();
	if(inpStr){
		char *start = inpStr;
		while(*inpStr){
			temp = *inpStr++;
			temp += value;
			value = temp << 10;
			temp += value;
			value = temp >> 6;
			value ^= temp;
		}
		delete[] start;
	}
	temp = value << 3;
	temp += value;
	unsigned int temp2 = temp >> 11;
	temp = temp2 ^ temp;
	temp2 = temp << 15;
	value = temp2 + temp;
	if(value < 2) value += 2;
	return value;
}

void *basicgen() {
	// WARNING: the following code is designed to give you a headache, but it probably won't
#if defined(WIN32)// && !defined(__GNUC__)
	const char * const *temp = (char **) (char ***) (char *********) 20;
	--temp = (char **) (char ****) 2000;
	temp = (char **) (char ****) 21241;
	int temp2 = (short) (unsigned) (size_t) 87938749U;
	temp2 >>= (int) (size_t) 20;
	temp2 <<= (int) (size_t) (long) 1;
	char *temp3 = getregszvalue(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Cryptography", "MachineGuid", KEY_WOW64_64KEY); // will fail on windows 2000
	if(temp3) return temp3;
	return (void *)getregszvalue(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Cryptography", "MachineGuid"); // will fail on 64-bit
	temp += temp2;
	temp2 -= (int)temp;
	return (void *)temp;
	/*
	void ****pguid = 2 + ( (void ****) (void **) (void ***) new GUID );
	CoCreateGuid((GUID *)(--pguid - 1)); // #include <Objbase.h>
	//pguid -= 0xF0F0;
	void *pt = new string;
	memset(pt, 0, sizeof((char *)pt)/sizeof(*(char *)pt));
	memcpy(pt, (void *)--pguid, sizeof(GUID));
	formatstring(pt)("%lu%hu%hu%d", ((GUID *)pguid)->Data1, ((GUID *)pguid)->Data3, ((GUID *)pguid)->Data2, *((GUID *)pguid)->Data4);
	delete (GUID *)pguid;
	conoutf("%s", pt);
	return pt;
	*/
	/*
	UUID u; // #pragma comment(lib, "Rpcrt4.lib")
			//#include <Rpc.h>
	switch(UuidCreateSequential (&u)){
		default: return NULL;
		case RPC_S_OK:
		case RPC_S_UUID_LOCAL_ONLY: // can we trust it?
			break;
	}
	char *pt = new string;
	formatstring(pt)("%lu%hu%hu%d", u.Data1, u.Data2, u.Data3, u.Data4);
	return pt;
	*/
#elif defined(__GNUC__) || defined(linux) || defined(__linux) || defined(__linux__) || defined(__APPLE__)
	char *pt = new string;
	formatstring(pt)("%lu", gethostid());
	return pt;
#else
	// OS not supported :(
	const char * const * temp = (char **)(char ***)20;
	--temp;
	temp = (char **)2000;
	return (void *)(char *)(temp = NULL);
#endif
}

const char *timestring(bool local, const char *fmt)
{
	static string asciitime;
	time_t t = time(NULL);
	struct tm * timeinfo;
	timeinfo = local ? localtime(&t) : gmtime (&t);
	strftime(asciitime, sizeof(string) - 1, fmt ? fmt : "%Y%m%d_%H.%M.%S", timeinfo); // sortable time for filenames
	return asciitime;
}

const char *asctime()
{
	return timestring(true, "%c");
}

const char *numtime()
{
	static string numt;
	formatstring(numt)("%ld", (long long) time(NULL));
	return numt;
}

char *path(char *s)
{
	for(char *t = s; (t = strpbrk(t, "/\\")); *t++ = PATHDIV);
	for(char *prevdir = NULL, *curdir = s;;)
	{
		prevdir = curdir[0]==PATHDIV ? curdir+1 : curdir;
		curdir = strchr(prevdir, PATHDIV);
		if(!curdir) break;
		if(prevdir+1==curdir && prevdir[0]=='.')
		{
			memmove(prevdir, curdir+1, strlen(curdir+1)+1);
			curdir = prevdir;
		}
		else if(curdir[1]=='.' && curdir[2]=='.' && curdir[3]==PATHDIV)
		{
			if(prevdir+2==curdir && prevdir[0]=='.' && prevdir[1]=='.') continue;
			memmove(prevdir, curdir+4, strlen(curdir+4)+1);
			curdir = prevdir;
		}
	}
	return s;
}

char *path(const char *s, bool copy)
{
	static string tmp;
	copystring(tmp, s);
	path(tmp);
	return tmp;
}

const char *behindpath(const char *s)
{
	const char *t = s;
	for( ; (s = strpbrk(s, "/\\")); t = ++s);
	return t;
}

const char *parentdir(const char *directory)
{
	const char *p = strrchr(directory, '/');
	if(!p) p = strrchr(directory, '\\');
	if(!p) p = directory;
	static string parent;
	size_t len = p-directory+1;
	copystring(parent, directory, len);
	return parent;
}

bool fileexists(const char *path, const char *mode)
{
	bool exists = true;
	if(mode[0]=='w' || mode[0]=='a') path = parentdir(path);
#ifdef WIN32
	if(GetFileAttributes(path) == INVALID_FILE_ATTRIBUTES) exists = false;
#else
	if(access(path, R_OK | (mode[0]=='w' || mode[0]=='a' ? W_OK : 0)) == -1) exists = false;
#endif
	return exists;
}

bool createdir(const char *path)
{
	size_t len = strlen(path);
	if(path[len-1]==PATHDIV)
	{
		static string strip;
		path = copystring(strip, path, len);
	}
#ifdef WIN32
	return CreateDirectory(path, NULL)!=0;
#else
	return mkdir(path, 0777)==0;
#endif
}

static void fixdir(char *dir)
{
	path(dir);
	size_t len = strlen(dir);
	if(dir[len-1]!=PATHDIV)
	{
		dir[len] = PATHDIV;
		dir[len+1] = '\0';
	}
}

void sethomedir(const char *dir)
{
	string tmpdir;
	copystring(tmpdir, dir);

#ifdef WIN32
	const char substitute[] = "?MYDOCUMENTS?";
	if(!strncmp(dir, substitute, strlen(substitute)))
	{
		const char *regpath = "Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders";
		char *mydocuments = getregszvalue(HKEY_CURRENT_USER, regpath, "Personal");
		if(mydocuments)
		{
			formatstring(tmpdir)("%s%s", mydocuments, dir+strlen(substitute));
			delete[] mydocuments;
		}
		else
		{
			printf("failed to retrieve 'Personal' path from '%s'\n", regpath);
		}
	}
#endif

	printf("Using home directory: %s\n", tmpdir);
	fixdir(copystring(homedir, tmpdir));
	createdir(homedir);
}

void addpackagedir(const char *dir)
{
	printf("Adding package directory: %s\n", dir);
	fixdir(packagedirs.add(newstringbuf(dir)));
}

const char *findfile(const char *filename, const char *mode)
{
	static string s;
	if(homedir[0])
	{
		formatstring(s)("%s%s", homedir, filename);
		if(fileexists(s, mode)) return s;
		if(mode[0]=='w' || mode[0]=='a')
		{
			string dirs;
			copystring(dirs, s);
			char *dir = strchr(dirs[0]==PATHDIV ? dirs+1 : dirs, PATHDIV);
			while(dir)
			{
				*dir = '\0';
				if(!fileexists(dirs, "r") && !createdir(dirs)) return s;
				*dir = PATHDIV;
				dir = strchr(dir+1, PATHDIV);
			}
			return s;
		}
	}
	if(mode[0]=='w' || mode[0]=='a') return filename;
	loopv(packagedirs)
	{
		formatstring(s)("%s%s", packagedirs[i], filename);
		if(fileexists(s, mode)) return s;
	}
	return filename;
}

int getfilesize(const char *filename)
{
	const char *found = findfile(filename, "rb");
	if(!found) return -1;
	FILE *fp = fopen(found, "rb");
	if(!fp) return -1;
	fseek(fp, 0, SEEK_END);
	int len = ftell(fp);
	fclose(fp);
	return len;
}

FILE *openfile(const char *filename, const char *mode)
{
	const char *found = findfile(filename, mode);
#ifndef STANDALONE
	if(mode && (mode[0]=='w' || mode[0]=='a')) conoutf("writing to file: %s", found);
#endif
	if(!found) return NULL;
	return fopen(found, mode);
}

gzFile opengzfile(const char *filename, const char *mode)
{
	const char *found = findfile(filename, mode);
#ifndef STANDALONE
	if(mode && (mode[0]=='w' || mode[0]=='a')) conoutf("writing to file: %s", found);
#endif
	if(!found) return NULL;
	return gzopen(found, mode);
}

char *loadfile(const char *fn, int *size, const char *mode)
{
	FILE *f = openfile(fn, mode ? mode : "rb");
	if(!f) return NULL;
	fseek(f, 0, SEEK_END);
	int len = ftell(f);
	if(len<=0) { fclose(f); return NULL; }
	fseek(f, 0, SEEK_SET);
	char *buf = new char[len+1];
	if(!buf) { fclose(f); return NULL; }
	buf[len] = 0;
	int rlen = (int)fread(buf, 1, len, f);
	fclose(f);
	if(len!=rlen && (!mode || strchr(mode, 'b')))
	{
		delete[] buf;
		return NULL;
	}
	if(size!=NULL) *size = rlen;
	return buf;
}

bool listdir(const char *dir, const char *ext, vector<char *> &files)
{
	int extsize = ext ? (int)strlen(ext)+1 : 0;
	#if defined(WIN32)
	defformatstring(pathname)("%s\\*.%s", dir, ext ? ext : "*");
	WIN32_FIND_DATA FindFileData;
	HANDLE Find = FindFirstFile(path(pathname), &FindFileData);
	if(Find != INVALID_HANDLE_VALUE)
	{
		do {
			files.add(newstring(FindFileData.cFileName, (int)strlen(FindFileData.cFileName) - extsize));
		} while(FindNextFile(Find, &FindFileData));
		return true;
	}
	#else
	string pathname;
	copystring(pathname, dir);
	DIR *d = opendir(path(pathname));
	if(d)
	{
		struct dirent *de;
		while((de = readdir(d)) != NULL)
		{
			if(!ext) files.add(newstring(de->d_name));
			else
			{
				int namelength = (int)strlen(de->d_name) - extsize;
				if(namelength > 0 && de->d_name[namelength] == '.' && strncmp(de->d_name+namelength+1, ext, extsize-1)==0)
					files.add(newstring(de->d_name, namelength));
			}
		}
		closedir(d);
		return true;
	}
	#endif
	else return false;
}

int listfiles(const char *dir, const char *ext, vector<char *> &files)
{
	int dirs = 0;
	if(listdir(dir, ext, files)) dirs++;
	string s;
	if(homedir[0])
	{
		formatstring(s)("%s%s", homedir, dir);
		if(listdir(s, ext, files)) dirs++;
	}
	loopv(packagedirs)
	{
		formatstring(s)("%s%s", packagedirs[i], dir);
		if(listdir(s, ext, files)) dirs++;
	}
	return dirs;
}

bool delfile(const char *path)
{
	return !remove(path);
}

extern ssqr *maplayout;
extern persistent_entity *mapents;
extern int maplayout_factor;

mapstats *loadmapstats(const char *filename, bool getlayout)
{
	static mapstats s;

	// following block should be put into an initializer?
	loopi(MAXENTTYPES) s.entcnt[i] = 0;
	loopi(3) s.spawns[i] = 0;
	loopi(3) s.flags[i] = 0;

	gzFile f = opengzfile(filename, "rb9");
	if(!f) return NULL;
	memset(&s.hdr, 0, sizeof(header));
#define INVALID_MAP { gzclose(f); return NULL; }
	if(gzread(f, &s.hdr, sizeof(header)-sizeof(int)*16-sizeof(char)*128)!=sizeof(header)-sizeof(int)*16-sizeof(char)*128 || (strncmp(s.hdr.head, "CUBE", 4) && strncmp(s.hdr.head, "ACMP",4) && strncmp(s.hdr.head, "ACRM",4))) INVALID_MAP
	endianswap(&s.hdr.version, sizeof(int), 4);
	if(s.hdr.version>MAPVERSION || s.hdr.numents > MAXENTITIES) INVALID_MAP
	if(s.hdr.version >=4 && gzread(f, &s.hdr.waterlevel, sizeof(int)*16)!=sizeof(int)*16) INVALID_MAP
	if(s.hdr.version >= 8 && gzread(f, &s.hdr.mediareq, sizeof(char)*128)!=sizeof(char)*128) INVALID_MAP
	if(s.hdr.version < 8)
		copystring(s.hdr.mediareq, "", 128);
	if(s.hdr.version>=4)
	{
		lilswap(&s.hdr.waterlevel, 1);
		lilswap(&s.hdr.maprevision, 2);
	}
	else
	{
		s.hdr.waterlevel = -100000;
		s.hdr.ambient = 0;
	}
	persistent_entity e;
	if(getlayout)
	{
		DELETEA(mapents);
		mapents = new persistent_entity[s.hdr.numents];
	}
	loopi(s.hdr.numents)
	{
		gzread(f, &e, sizeof(persistent_entity));
		endianswap(&e, sizeof(short), 4);
		TRANSFORMOLDENTITIES(s.hdr)
		if(e.type == PLAYERSTART && (e.attr2 == 0 || e.attr2 == 1 || e.attr2 == 100)) ++s.spawns[e.attr2 == 100 ? 2 : e.attr2];
		if(e.type == CTF_FLAG && e.attr2 >= 0 && e.attr2 <= 2) { ++s.flags[e.attr2]; if(e.attr2 < 2) s.flagents[e.attr2] = i; }
		s.entcnt[e.type]++;
		if(getlayout) mapents[i] = e;
	}
	if(getlayout)
	{
		DELETEA(maplayout);
		if(s.hdr.sfactor <= LARGEST_FACTOR && s.hdr.sfactor >= SMALLEST_FACTOR)
		{
			maplayout_factor = s.hdr.sfactor;
			int layoutsize = 1 << (maplayout_factor * 2);
			bool fail = false;
			maplayout = new ssqr[layoutsize + 256];
			memset(maplayout, 0, layoutsize * sizeof(ssqr));
			ssqr *t = NULL;
			//char floor = 0, ceil;
			loopk(layoutsize)
			{
				ssqr &sq = maplayout[k];
				sq.type = gzgetc(f);
				switch(sq.type)
				{
					case 255:
					{
						int n = gzgetc(f);
						if(!t || n < 0) { fail = true; break; }
						loopi(n) memcpy(&sq + i, t, sizeof(ssqr));
						k += n - 1;
						break;
					}
					case 254: // only in MAPVERSION<=2
						if(!t) { fail = true; break; }
						memcpy(&sq, t, sizeof(ssqr));
						gzgetc(f); gzgetc(f);
						break;
					default:
						if(/*sq.type<0 ||*/ sq.type>=MAXTYPE)  { fail = true; break; }
						sq.floor = gzgetc(f);
						sq.ceil = gzgetc(f);
						if(sq.floor >= sq.ceil && sq.ceil > -128) sq.floor = sq.ceil - 1;  // for pre 12_13
						sq.wtex = gzgetc(f);
						gzgetc(f); // ftex
						sq.ctex = gzgetc(f);
						if(s.hdr.version<=2) { gzgetc(f); gzgetc(f); }
						sq.vdelta = gzgetc(f);
						if(s.hdr.version>=2) gzgetc(f); // utex
						if(s.hdr.version>=5) gzgetc(f); // tag
						break;
					case SOLID:
						sq.floor = 0;
						sq.ceil = 16;
						sq.wtex = gzgetc(f);
						sq.vdelta = gzgetc(f);
						if(s.hdr.version<=2) { gzgetc(f); gzgetc(f); }
						break;
				}
				if(fail) break;
				t = &sq;
			}
			if(fail) DELETEA(maplayout);
		}
	}
	gzclose(f);
	s.cgzsize = getfilesize(filename);
	return &s;
}

///////////////////////// debugging ///////////////////////

#if defined(WIN32) && !defined(_DEBUG) && !defined(__GNUC__)
void stackdumper(unsigned int type, EXCEPTION_POINTERS *ep)
{
	if(!ep) fatal("unknown type");
	EXCEPTION_RECORD *er = ep->ExceptionRecord;
	CONTEXT *context = ep->ContextRecord;
	string out, t;
	formatstring(out)("Win32 Exception: 0x%x [0x%x]\n\n", er->ExceptionCode, er->ExceptionCode==EXCEPTION_ACCESS_VIOLATION ? er->ExceptionInformation[1] : -1);
	STACKFRAME sf = {{context->Eip, 0, AddrModeFlat}, {}, {context->Ebp, 0, AddrModeFlat}, {context->Esp, 0, AddrModeFlat}, 0};
	SymInitialize(GetCurrentProcess(), NULL, TRUE);

	while(::StackWalk(IMAGE_FILE_MACHINE_I386, GetCurrentProcess(), GetCurrentThread(), &sf, context, NULL, ::SymFunctionTableAccess, ::SymGetModuleBase, NULL))
	{
		struct { IMAGEHLP_SYMBOL sym; string n; } si = { { sizeof( IMAGEHLP_SYMBOL ), 0, 0, 0, sizeof(string) } };
		IMAGEHLP_LINE li = { sizeof( IMAGEHLP_LINE ) };
		DWORD off;
		if(SymGetSymFromAddr(GetCurrentProcess(), (DWORD)sf.AddrPC.Offset, &off, &si.sym) && SymGetLineFromAddr(GetCurrentProcess(), (DWORD)sf.AddrPC.Offset, &off, &li))
		{
			char *del = strrchr(li.FileName, '\\');
			formatstring(t)("%s - %s [%d]\n", si.sym.Name, del ? del + 1 : li.FileName, li.LineNumber);
			concatstring(out, t);
		}
	}
	fatal("%s", out);
}
#elif defined(linux) || defined(__linux) || defined(__linux__)

#include <execinfo.h>

// stack dumping on linux, inspired by Sachin Agrawal's sample code

struct signalbinder
{
	static void stackdumper(int sig)
	{
		printf("stacktrace:\n");

		const int BTSIZE = 25;
		void *array[BTSIZE];
		int n = backtrace(array, BTSIZE);
		char **symbols = backtrace_symbols(array, n);
		for(int i = 0; i < n; i++)
		{
			printf("%s\n", symbols[i]);
		}
		free(symbols);

		fatal("ACR error (%d)", sig);

	}

	signalbinder()
	{
		// register signals to dump the stack if they are raised,
		// use constructor for early registering
		signal(SIGSEGV, stackdumper);
		signal(SIGFPE, stackdumper);
		signal(SIGILL, stackdumper);
		signal(SIGBUS, stackdumper);
		signal(SIGSYS, stackdumper);
		signal(SIGABRT, stackdumper);
	}
};

signalbinder sigbinder;

#endif


///////////////////////// misc tools ///////////////////////

bool cmpb(void *b, int n, enet_uint32 c)
{
	ENetBuffer buf;
	buf.data = b;
	buf.dataLength = n;
	return enet_crc32(&buf, 1)==c;
}

bool cmpf(char *fn, enet_uint32 c)
{
	int n = 0;
	char *b = loadfile(fn, &n);
	bool r = cmpb(b, n, c);
	delete[] b;
	return r;
}

void endianswap(void *memory, int stride, int length)   // little endian as storage format
{
	static const int littleendian = 1;
	if(!*(const char *)&littleendian) loop(w, length) loop(i, stride/2)
	{
		uchar *p = (uchar *)memory+w*stride;
		uchar t = p[i];
		p[i] = p[stride-i-1];
		p[stride-i-1] = t;
	}
}

bool isbigendian()
{
	return !*(const uchar *)&islittleendian;
}

void strtoupper(char *t, const char *s)
{
	if(!s) s = t;
	while(*s)
	{
		*t = toupper(*s);
		t++; s++;
	}
	*t = '\0';
}

const char *atoip(const char *s, enet_uint32 *ip)
{
	unsigned int d[4];
	int n;
	if(!s || sscanf(s, "%u.%u.%u.%u%n", d, d + 1, d + 2, d + 3, &n) != 4) return NULL;
	*ip = 0;
	loopi(4)
	{
		if(d[i] > 255) return NULL;
		*ip = (*ip << 8) + d[i];
	}
	return s + n;
}

const char *atoipr(const char *s, iprange *ir)
{
	if((s = atoip(s, &ir->lr)) == NULL) return NULL;
	ir->ur = ir->lr;
	s += strspn(s, " \t");
	if(*s == '-')
	{
		if(!(s = atoip(s + 1, &ir->ur)) || ir->lr > ir->ur) return NULL;
	}
	else if(*s == '/')
	{
		int m, n;
		if(sscanf(s + 1, "%d%n", &m, &n) != 1 || m < 0 || m > 32) return NULL;
		unsigned long bm = (1 << (32 - m)) - 1;
		ir->lr &= ~bm;
		ir->ur |= bm;
		s += 1 + n;
	}
	return s;
}

const char *iptoa(const enet_uint32 ip)
{
	static string s[2];
	static int buf = 0;
	buf = (buf + 1) % 2;
	formatstring(s[buf])("%d.%d.%d.%d", (ip >> 24) & 255, (ip >> 16) & 255, (ip >> 8) & 255, ip & 255);
	return s[buf];
}

const char *iprtoa(const struct iprange &ipr)
{
	static string s[2];
	static int buf = 0;
	buf = (buf + 1) % 2;
	if(ipr.lr == ipr.ur) copystring(s[buf], iptoa(ipr.lr));
	else formatstring(s[buf])("%s-%s", iptoa(ipr.lr), iptoa(ipr.ur));
	return s[buf];
}

int cmpiprange(const struct iprange *a, const struct iprange *b)
{
	if(a->lr < b->lr) return -1;
	if(a->lr > b->lr) return 1;
	return 0;
}

int cmpipmatch(const struct iprange *a, const struct iprange *b)
{
	return - (a->lr < b->lr) + (a->lr > b->ur);
}

char *concatformatstring(char *d, const char *s, ...)
{
	static s_sprintfdv(temp, s);
	return concatstring(d, temp);
}

const char *hiddenpwd(const char *pwd, int showchars)
{
	static int sc = 3;
	static string text;
	copystring(text, pwd);
	if(showchars > 0) sc = showchars;
	for(int i = (int)strlen(text) - 1; i >= sc; --i) text[i] = '*';
	return text;
}
//////////////// geometry utils ////////////////

static inline float det2x2(float a, float b, float c, float d) { return a*d - b*c; }
static inline float det3x3(float a1, float a2, float a3,
						   float b1, float b2, float b3,
						   float c1, float c2, float c3)
{
	return a1 * det2x2(b2, b3, c2, c3)
		 - b1 * det2x2(a2, a3, c2, c3)
		 + c1 * det2x2(a2, a3, b2, b3);
}

float glmatrixf::determinant() const
{
	float a1 = v[0], a2 = v[1], a3 = v[2], a4 = v[3],
		  b1 = v[4], b2 = v[5], b3 = v[6], b4 = v[7],
		  c1 = v[8], c2 = v[9], c3 = v[10], c4 = v[11],
		  d1 = v[12], d2 = v[13], d3 = v[14], d4 = v[15];

	return a1 * det3x3(b2, b3, b4, c2, c3, c4, d2, d3, d4)
		 - b1 * det3x3(a2, a3, a4, c2, c3, c4, d2, d3, d4)
		 + c1 * det3x3(a2, a3, a4, b2, b3, b4, d2, d3, d4)
		 - d1 * det3x3(a2, a3, a4, b2, b3, b4, c2, c3, c4);
}

void glmatrixf::adjoint(const glmatrixf &m)
{
	float a1 = m.v[0], a2 = m.v[1], a3 = m.v[2], a4 = m.v[3],
		  b1 = m.v[4], b2 = m.v[5], b3 = m.v[6], b4 = m.v[7],
		  c1 = m.v[8], c2 = m.v[9], c3 = m.v[10], c4 = m.v[11],
		  d1 = m.v[12], d2 = m.v[13], d3 = m.v[14], d4 = m.v[15];

	v[0]  =  det3x3(b2, b3, b4, c2, c3, c4, d2, d3, d4);
	v[1]  = -det3x3(a2, a3, a4, c2, c3, c4, d2, d3, d4);
	v[2]  =  det3x3(a2, a3, a4, b2, b3, b4, d2, d3, d4);
	v[3]  = -det3x3(a2, a3, a4, b2, b3, b4, c2, c3, c4);

	v[4]  = -det3x3(b1, b3, b4, c1, c3, c4, d1, d3, d4);
	v[5]  =  det3x3(a1, a3, a4, c1, c3, c4, d1, d3, d4);
	v[6]  = -det3x3(a1, a3, a4, b1, b3, b4, d1, d3, d4);
	v[7]  =  det3x3(a1, a3, a4, b1, b3, b4, c1, c3, c4);

	v[8]  =  det3x3(b1, b2, b4, c1, c2, c4, d1, d2, d4);
	v[9]  = -det3x3(a1, a2, a4, c1, c2, c4, d1, d2, d4);
	v[10] =  det3x3(a1, a2, a4, b1, b2, b4, d1, d2, d4);
	v[11] = -det3x3(a1, a2, a4, b1, b2, b4, c1, c2, c4);

	v[12] = -det3x3(b1, b2, b3, c1, c2, c3, d1, d2, d3);
	v[13] =  det3x3(a1, a2, a3, c1, c2, c3, d1, d2, d3);
	v[14] = -det3x3(a1, a2, a3, b1, b2, b3, d1, d2, d3);
	v[15] =  det3x3(a1, a2, a3, b1, b2, b3, c1, c2, c3);
}

bool glmatrixf::invert(const glmatrixf &m, float mindet)
{
	float a1 = m.v[0], b1 = m.v[4], c1 = m.v[8], d1 = m.v[12];
	adjoint(m);
	float det = a1*v[0] + b1*v[1] + c1*v[2] + d1*v[3]; // float det = m.determinant();
	if(fabs(det) < mindet) return false;
	float invdet = 1/det;
	loopi(16) v[i] *= invdet;
	return true;
}

void vecfromyawpitch(float yaw, float pitch, int move, int strafe, vec &m)
{
    if(move)
    {
        m.x = move*-sinf(RAD*yaw);
        m.y = move*cosf(RAD*yaw);
    }
    else m.x = m.y = 0;

    if(pitch)
    {
        m.x *= cosf(RAD*pitch);
        m.y *= cosf(RAD*pitch);
        m.z = move*sinf(RAD*pitch);
    }
    else m.z = 0;

    if(strafe)
    {
        m.x += strafe*cosf(RAD*yaw);
        m.y += strafe*sinf(RAD*yaw);
    }
}

void vectoyawpitch(const vec &v, float &yaw, float &pitch)
{
    yaw = 180-atan2(v.x, v.y)/RAD; // +180
    pitch = asin(v.z/v.magnitude())/RAD;
}

void fixfullrange(float &yaw, float &pitch, float &roll, bool full)
{
    if(full)
    {
        while(pitch < -180.0f) pitch += 360.0f;
        while(pitch >= 180.0f) pitch -= 360.0f;
        while(roll < -180.0f) roll += 360.0f;
        while(roll >= 180.0f) roll -= 360.0f;
    }
    else
    {
        if(pitch > 89.9f) pitch = 89.9f;
        if(pitch < -89.9f) pitch = -89.9f;
        if(roll > 89.9f) roll = 89.9f;
        if(roll < -89.9f) roll = -89.9f;
    }
    while(yaw < 0.0f) yaw += 360.0f;
    while(yaw >= 360.0f) yaw -= 360.0f;
}

void fixrange(float &yaw, float &pitch)
{
    float r = 0.f;
    fixfullrange(yaw, pitch, r, false);
}

void getyawpitch(const vec &from, const vec &pos, float &yaw, float &pitch)
{
    float dist = from.dist(pos);
    yaw = 180-atan2(pos.x-from.x, pos.y-from.y)/RAD; // +180
    pitch = asin((pos.z-from.z)/dist)/RAD;
}

void scaleyawpitch(float &yaw, float &pitch, float targyaw, float targpitch, float yawspeed, float pitchspeed, float rotate)
{
    if(yaw < targyaw-180.0f) yaw += 360.0f;
    if(yaw > targyaw+180.0f) yaw -= 360.0f;
    float offyaw = (rotate < 0 ? fabs(rotate) : (rotate > 0 ? min(float(fabs(targyaw-yaw)), rotate) : fabs(targyaw-yaw)))*yawspeed,
        offpitch = (rotate < 0 ? fabs(rotate) : (rotate > 0 ? min(float(fabs(targpitch-pitch)), rotate) : fabs(targpitch-pitch)))*pitchspeed;
    if(targyaw > yaw)
    {
        yaw += offyaw;
        if(targyaw < yaw) yaw = targyaw;
    }
    else if(targyaw < yaw)
    {
        yaw -= offyaw;
        if(targyaw > yaw) yaw = targyaw;
    }
    if(targpitch > pitch)
    {
        pitch += offpitch;
        if(targpitch < pitch) pitch = targpitch;
    }
    else if(targpitch < pitch)
    {
        pitch -= offpitch;
        if(targpitch > pitch) pitch = targpitch;
    }
    fixrange(yaw, pitch);
}

#ifndef STANDALONE
SVARP(locale, "en");

struct langdef { string key; string val; };
vector<langdef> langdefs;

langdef *findlang(const char *key){
	if(key) loopv(langdefs) if(!strcasecmp(langdefs[i].key, key)) return &langdefs[i];
	return NULL;
}

void deflang(char *key, char *val){
	if(!key || !val || !*key) return;
	langdef *langfound = findlang(key);

	if(!langfound) langfound = &langdefs.add();
	copystring(langfound->key, key);
	copystring(langfound->val, val);
}
COMMANDN(lang, deflang, ARG_2STR);

const char *_gettext(const char *msgid)
{
	langdef *langfound = findlang(msgid);
	if(langfound) return langfound->val;
	return msgid;
}

void getlang(const char *key){
	if(key) result(_(key));
}
COMMAND(getlang, ARG_1STR);
#endif