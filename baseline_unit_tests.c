#include <stdio.h>
#include <stdlib.h>
#include <curl/curl.h>
#import <unistd.h>

int basic_test(CURL*, CURL*, CURLcode, int);
int fill_cache(CURL*, CURL*, CURLcode, int);
void run_diff(char*, char*);
char **get_filenames(int);

static size_t header_callback(char *buffer, size_t size, size_t nitems, void *userdata)
{
	/* received header is nitems * size long in 'buffer' NOT ZERO TERMINATED */
	/* 'userdata' is set with CURLOPT_HEADERDATA */
	size_t numbytes = size * nitems;
    printf("%.*s", numbytes, buffer);
    //fputs(buffer, )
    return numbytes;
}

int main(int argc, char **argv) {
	CURL *curl = curl_easy_init();
	CURL *proxy = curl_easy_init();
	CURLcode res;
	int num_files = 0;

	// setup CURL objects		
	curl_easy_setopt(proxy, CURLOPT_PROXY, "localhost");
	curl_easy_setopt(proxy, CURLOPT_PROXYPORT, 9100L);

	// run tests
	printf("test base case:\n");
	num_files = basic_test(curl, proxy, res, num_files);
	
	printf("test cache:\n");
	num_files = basic_test(curl, proxy, res, num_files);
	
	printf("fill cache:\n");
	num_files = fill_cache(curl, proxy, res, num_files);
	
	printf("request first page again (should no longer be cached):\n");
	num_files = basic_test(curl, proxy, res, num_files);

	curl_easy_cleanup(curl);
	
	return 0;
}

void run_diff(char *curl_file, char *proxy_file) {
	char cmd[100];
	sprintf(cmd, "diff %s %s", curl_file, proxy_file);
	printf("%s\n", cmd);
	system(cmd);
}

int basic_test(CURL *curl, CURL *proxy, CURLcode res, int num_files) {
	char **filenames = get_filenames(num_files);
	FILE *curl_file = fopen(filenames[0], "w");
	FILE *proxy_file = fopen(filenames[1], "w");
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, curl_file);
	curl_easy_setopt(proxy, CURLOPT_WRITEDATA, proxy_file);

	curl_easy_setopt(curl, CURLOPT_URL, "http://www.cs.tufts.edu/comp/112/index.html");
	res = curl_easy_perform(curl);
	curl_easy_setopt(proxy, CURLOPT_URL, "http://www.cs.tufts.edu/comp/112/index.html");
	res = curl_easy_perform(proxy);

	run_diff(filenames[0], filenames[1]);
	return num_files+1;
}

int fill_cache(CURL *curl, CURL *proxy, CURLcode res, int num_files) {
	int num = num_files;
	char *pages[10] = {"http://www.cs.cmu.edu/~prs/bio.html", "http://stevenbell.me/about/",
						"http://linux.die.net/man/1/curl", "http://www.ece.tufts.edu/es/4/index.html",
						"http://www.ece.tufts.edu/es/4/labs.html", "http://www.cs.tufts.edu/comp/112/assignments.html",
						"http://www.cs.cmu.edu/~prs/index.html", "http://www.york.ac.uk/teaching/cws/wws/webpage1.html",
						"http://www.cs.cmu.edu/~dga/dga-headshot.jpg", "http://www.ece.tufts.edu/ee/107/"};

	for (int i = 0; i < 10; i++) {
		printf("requesting %s\n", pages[i]);

		char **filenames = get_filenames(num);
		num++;
		FILE *curl_file = fopen(filenames[0], "w");
		FILE *proxy_file = fopen(filenames[1], "w");
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, curl_file);
		curl_easy_setopt(proxy, CURLOPT_WRITEDATA, proxy_file);

		curl_easy_setopt(curl, CURLOPT_URL, pages[i]);
		res = curl_easy_perform(curl);
		curl_easy_setopt(proxy, CURLOPT_URL, pages[i]);
		res = curl_easy_perform(proxy);
		//sleep(5);
		run_diff(filenames[0], filenames[1]);
	}
	return num;
}

char **get_filenames(int file_num) {
	char *curl_name = malloc(sizeof("curl_outputxxx.txt"));
	char *proxy_name = malloc(sizeof("proxy_outputxxx.txt"));
	sprintf(curl_name, "curl_output%d.txt", file_num);
	sprintf(proxy_name, "proxy_output%d.txt", file_num);
	char **ray = malloc(2*sizeof(curl_name));
	ray[0] = curl_name;
	ray[1] = proxy_name;
	return ray;
}