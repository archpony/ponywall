#define _XOPEN_SOURCE 700
#define _DEFAULT_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <time.h>
#include <argp.h>
#include <sys/stat.h>
#include <curl/curl.h>
#include <json-c/json.h>


#define BING_COM "https://www.bing.com"
#define BING_URL "https://www.bing.com/HPImageArchive.aspx?format=js&idx=0&n=1&mkt=en-US"

#define CFG_DIR     "ponywall"
#define CFG_FILE    "pwall.dat"
#define WP_FILE     "pwall.jpg"
#define CP_FILE     "pwall.txt"
#define WP_CMD_DEF  "feh --bg-fill %s"
#define DATE_FMT	"%Y%m%d%H%M"
#define TIMEOUT_DEF  900

struct wpdata {
	char *url;
	char *lsd;
};

struct memory {
	char *response;
	size_t size;
};

char CFG_HOME[128]; 
char *WP_CMD = WP_CMD_DEF;

CURL *curl;
CURLcode res;

char *screen_res = NULL;
int force_update = 0;
int auto_mode = 0;
int timeout = TIMEOUT_DEF;

void init_config();
struct wpdata *read_config();
void save_config(struct wpdata *wpd);
void save_title(char *title);
void get_cur_date();

static size_t mem_write(void *data, size_t size, size_t nmemb, void *userp)
{
	size_t realsize = size * nmemb;
	struct memory *mem = (struct memory *)userp;

	char *ptr = realloc(mem->response, mem->size + realsize + 1);
	if(ptr == NULL)
		return 0;

	mem->response = ptr;
	memcpy(&(mem->response[mem->size]), data, realsize);
	mem->size += realsize;
	mem->response[mem->size] = 0;

	return realsize;
}

static size_t file_write(void *data, size_t size, size_t nmemb, void *stream)
{
  size_t written = fwrite(data, size, nmemb, (FILE *)stream);
  return written;
}

void parse_args(int argc, char *argv[])
{
	int opt;
	while ((opt = getopt(argc, argv, "hfas:c:t:")) != -1) {
		switch (opt) {
			case 'h':
				fprintf(stderr, "Usage: %s [-s <screen size>] [-f]\
 [-c <set wp command>] [-a] [-t <timeout seconds>]\n",
						argv[0]);
				exit(0);
			case 'a':
				auto_mode = 1;
				break;
			case 'f':
				force_update = 1;
				break;
			case 's':
				screen_res = optarg;
				break;
			case 't':
				timeout = atoi(optarg);
				if (timeout==0) {
					timeout = TIMEOUT_DEF;
					printf("Wrong value for timeout: %s\n", optarg);
				}
				break;
			case 'c':
				WP_CMD = optarg;
				break;
		}
	}
}

int get_wp_data(struct memory *chunk)
{
	curl_easy_setopt(curl, CURLOPT_URL, BING_URL);
	chunk->response = NULL;
	chunk->size = 0;
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, mem_write);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)chunk);

	res = curl_easy_perform(curl);
	if(res != CURLE_OK) {
		fprintf(stderr, "curl_easy_perform() failed: %s\n",
				curl_easy_strerror(res));
		return 1;
	}
	return 0;
}

void get_json_str(json_object *jimg, const char *field, char **data)
{
	json_object *jo = json_object_object_get(jimg, field);
	const char *str = json_object_get_string(jo);
	size_t len = strlen(str);
	*data = (char *)malloc(len+1);
	strcpy(*data, str);
	//json_object_put(jo);   //may be its wrong
}

int download_wallp(char *wallp_url)
{
	char wpf[256];
	sprintf(wpf,"%s/%s",CFG_HOME, WP_FILE);
	FILE *fp = fopen(wpf,"wb");
	if (fp==NULL) {
		perror("Cannot save to wallpaper file");
		return -1;
	}
	curl_easy_setopt(curl, CURLOPT_URL, wallp_url);
	//curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, file_write);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
	res = curl_easy_perform(curl);
	if(res != CURLE_OK) {
		fprintf(stderr, "curl_easy_perform() failed: %s\n",
				curl_easy_strerror(res));
		return 1;
	}
	fclose(fp);
	return 0;
}

void make_url(char *def_url, char *base_url, char *wallp_url)
{
	*wallp_url='\0';
	if (screen_res != NULL) {
		sprintf(wallp_url,"%s%s_%s.jpg", 
				BING_COM,
				base_url, 
				screen_res);
	} else {
		sprintf(wallp_url,"%s%s_1920x1080.jpg", 
				BING_COM,
				base_url); //cannot use def_url because HTTP 400 error 
	}
}

void make_next_time(char *datestr)
{
	struct tm sdate = {0};
	char *pres = strptime(datestr, DATE_FMT, &sdate);
	if (pres==NULL) {
		printf("Error in datetime %s\n", datestr);
		return;
	}
	sdate.tm_mday++;
	mktime(&sdate);
	sprintf(datestr,"%d%02d%02d%02d%02d",
			sdate.tm_year+1900, sdate.tm_mon+1, sdate.tm_mday,
			sdate.tm_hour, sdate.tm_min);
}

int is_expired(struct wpdata *config)
{
	if (config==NULL||force_update)
		return 1;
	struct tm sdate = {0};
	char *pres = strptime(config->lsd, DATE_FMT, &sdate);
	if (pres==NULL) {
		return 1;  //if the date is wrong we need an update anyway
	}
    time_t src_time = timegm(&sdate);
	time_t cur_time = time(NULL);
	double diff = difftime(cur_time, src_time);
	return diff>0;
}

void exec_cmd()
{
	char *wp_name;
	size_t len;
	char *cmd;

	len = strlen(CFG_HOME);
	len += strlen(WP_FILE);
	wp_name = malloc(len + 2); //count separator char too
	sprintf(wp_name, "%s/%s", CFG_HOME, WP_FILE);
	len += strlen(WP_CMD);
	cmd = malloc(len + 2);
	sprintf(cmd, WP_CMD, wp_name); 
	system(cmd);
	free(cmd);
	free(wp_name);
}

int wp_request(struct wpdata *wpd)
{
	struct memory chunk;

	int res = get_wp_data(&chunk);
	if (res != 0) {
		puts("Cannot continue.");
		return res;
	}

	json_object *root = json_tokener_parse(chunk.response);        
	json_object *images = json_object_object_get(root, "images");

	char *img_url;
	char *img_default_url;
	char *img_title;
	char *startdate;

	int n = json_object_array_length(images);
	for (int i=0; i<n; i++) {
		json_object *jimg = json_object_array_get_idx(images, i);
		get_json_str(jimg, "urlbase", &img_url);
		get_json_str(jimg, "url", &img_default_url);
		get_json_str(jimg, "copyright", &img_title);
		get_json_str(jimg, "fullstartdate", &startdate);
	}

	json_object_put(root);

	make_next_time(startdate);

	char wallp_url[2048];
	make_url(img_default_url, img_url, wallp_url);

	printf("Wallp:%s\n",img_title);
	printf("Url:%s\n", wallp_url);

	res = download_wallp(wallp_url);
	if (res!=0) return res;

	wpd->url = wallp_url;
	wpd->lsd = startdate;
	save_config(wpd);
	save_title(img_title);

    exec_cmd();

	free(img_url);
	free(img_title);
	free(startdate);

	return 0;
}

int main(int argc, char **argv)
{
	parse_args(argc, argv);

	init_config();
	struct wpdata *wpd = read_config();
	
	if (!is_expired(wpd)&&!auto_mode) {
		puts("Wallpaper is up to date");
		return 0;
	}

	if (wpd==NULL) {
		puts("No config, start from scratch");
		wpd = malloc(sizeof(struct wpdata));
	}

	curl_global_init(CURL_GLOBAL_DEFAULT);

	curl = curl_easy_init();
	if(!curl) {
		puts("Cannot use CURL, exiting.");
		return -1;
	}
	int wpres;
	if (auto_mode) {
		while(1) {
			sleep(timeout); 
			if (is_expired(wpd)) {
				wpres = wp_request(wpd);
				if (wpres) printf("Got %d error.\n", wpres);
			}
		}
	} else {
    	wpres = wp_request(wpd);
	}
	
    
	curl_easy_cleanup(curl);
	curl_global_cleanup();
	return wpres;
}


void init_config()
{
	char *cfg_base = getenv("XDG_CONFIG_HOME");
	if (cfg_base==NULL) {
		char *home = getenv("HOME");
		sprintf(CFG_HOME, "%s/.config", home);
	} else {
		strcpy(CFG_HOME, cfg_base);
	}
	strcat(CFG_HOME, "/");
	strcat(CFG_HOME, CFG_DIR);

	struct stat st = {0};
	if (stat(CFG_HOME, &st) == -1) {
		mkdir(CFG_HOME, 0755);
	}
}


struct wpdata *read_config()
{
	char fname[128];
	const size_t bsize = 256;
	sprintf(fname, "%s/%s", CFG_HOME, CFG_FILE);

	FILE *cfg = fopen(fname, "rt");
	if (cfg==NULL) return NULL;
	char buf[bsize];
	char *res = fgets(buf, bsize, cfg);
	if (res==NULL) return NULL;
	res[strcspn(res, "\n")] = 0;
	struct wpdata *wpd = malloc(sizeof(struct wpdata));
	size_t len = strlen(buf)+1;
	wpd->url = malloc(sizeof(char)*len);
	strcpy(wpd->url, buf);

	res = fgets(buf, bsize, cfg);
	if (res==NULL) return NULL;
	res[strcspn(res, "\n")] = 0;
	len = strlen(buf)+1;
	wpd->lsd = malloc(sizeof(char)*len);
	strcpy(wpd->lsd, buf);

	fclose(cfg);
	return wpd;
}

void save_config(struct wpdata *wpd)
{
	char fname[128];
	sprintf(fname, "%s/%s", CFG_HOME, CFG_FILE);
	FILE *cfg = fopen(fname, "wt");
	if (!cfg) {
		perror("Cannot save config!");
		return;
	}

	fputs(wpd->url, cfg); fputs("\n", cfg);
	fputs(wpd->lsd, cfg); fputs("\n", cfg);
	fclose(cfg);
}


void save_title(char *title)
{
	char fname[128];
	sprintf(fname, "%s/%s", CFG_HOME, CP_FILE);

	FILE *cfg = fopen(fname, "wt");
	if (!cfg) {
		perror("Cannot save title file!");
		return;
	}

	fputs(title, cfg);
	fclose(cfg);
}

