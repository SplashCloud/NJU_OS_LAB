#include <iostream>
#include <string.h>
#include <stdio.h>
#include <string>

using namespace std;

/**
 * 总体思路:
 * 1. 搭建I/O框架: 读取输入、解析输入...
 * 2. 加载 img 文件: 读取boot部分获得基本信息、并且形成文件结构
 * 3. 处理ls命令
 * 4. 处理cat命令
 */
// =============================================================================================

#define NAME_LEN 32
#define BUF_LEN 512
#define ENTRY_SIZE 32
#define SECTOR_SIZE 512
#define FILES_NUM 1000
#define FILE_SIZE 10240
#define SLASH_LEN 32

typedef unsigned char b;   // 1 byte
typedef unsigned short w;  // word, 2 byte
typedef unsigned int dw;   // double word, 4 byte

#pragma pack(1)
typedef struct Boot{
	w	bytes_per_sector; // 每个扇区的字节数
	b	sectors_per_cluster; // 每个簇的扇区数
	w	reserved_sectors; // boot record 占用的扇区数
	b	num_of_fat; // FAT 数量
	w	num_of_dir_entry; // 根目录文件数
	w	total_sectors; // 扇区数
	b	media;
	w 	sector_per_fat; // 每个fat占用扇区数
} Boot;
#pragma pack()

typedef struct Entry{
	char	name[11];
	b		attr;
	char 	reserved[10];
	w		mod_time;
	w		mod_date;
	w		fst_cluster;
	dw		size;
} Entry;

typedef struct File_Node{
	string name; // 文件/目录名
	string full_name; // 全路径名
	int attr; // 0 目录 1 文件 -1 分割点 2: . 3: ..
	int fst_cluster; // 在数据区开始簇号
	int size; // 文件/目录大小bytes
	int child_beign; // 子目录/文件开始位置
	int data_beign; // 在数据区的开始偏移
	int father_pos; // 父节点的位置
	int direct_dirs; // 直接子目录数
	int direct_files; // 直接子文件数
} fn;

// =========================================================================================
static const char* img_path = "../a.img";
static const char* exit_string = "exit";
Boot boot;
bool has_l = false;
string name;

int dir_entry_sectors; // 根目录的扇区数
int sectors_before_dir; // 根目录前的扇区数
int sectors_before_data; // 数据区前的扇区数
int nums_of_dir_entry; // 根目录区的entry数
int bytes_of_fat; // fat的字节数

fn *files[FILES_NUM]; // 存放所有文件和目录

int files_idx = 0;

// ===== error info
static const char *cmd_error = "error_command!\n";
static const char *not_dir_error = "ls: not a directory...\n";
static const char *not_file_error = "cat: not a file...\n";
static const char *cant_find_file_error = "cat: can't find the file...\n";
static const char *cant_find_dir_error = "ls: can't find the dir...\n";

// ========================================================================================
extern "C"{
	void print(const char *s, int len);
}
int deal_input();
void load_img();
void deal_ls();
void deal_cat();
string entry_name_toString(const char *s);
bool cmp_name(const string s1, const char *s2);
void read_boot(FILE* fp);
void init_files();
void read_all_files(FILE* fp, int father, int start, int ents);
void end_dir();
void unflod_dir(fn *dir);
void print_content(fn* file, FILE* fp);

// ========================================================================================

// ============ 工具函数 =========

void print_string(string str){
	const char * s = str.c_str();
	int len = strlen(s);
	print(s, len);
}

void print_num(int n){
	if(n == 0) print("0", 1);
	char nums[20];
	int i = 0;
	while(n != 0){
		int x = n % 10;
		nums[i++] = x + 48;
		n /= 10;
	}
	nums[i] = '\0';
	for(int j = 0; j < i / 2; j ++){
		int tmp = nums[j];
		nums[j] = nums[i - j - 1];
		nums[i - j - 1] = tmp;
	}
	print(nums, i);
}

void print_newLine(){
	print("\n", 1);
}

void print_blank(){
	print(" ", 1);
}

void print_red(string str){
	print_string("\033[31m");
	print_string(str);
	print_string("\033[0m");
}

string entry_name_toString(const char *s){
	string name = "";
    for(int i = 0; i < 8; i++){
        if(s[i] == ' ') continue;
        name += s[i];
    }
    if(s[8] != ' ') { // 文件
        name += '.';
        for(int i = 8; i < 11; i++){
            if(s[i] == ' ') continue;
            name += s[i];
        }
    }
    return name;
}

bool cmp_name(const string s1, const char *s2){
    string entry_name = entry_name_toString(s2);
	return s1 == entry_name;
}


// ============ 核心函数 =========

int deal_input(){
	name = ""; has_l = false;
	string buf = "";
	int cnt = 0;
	int cmd_code = -1;
	int set_file = false;
	getline(cin, buf);
	int i = 0;
	while( i < buf.size() ){

		int j = i;
		for(; i < buf.size() && buf[i] != ' '; i++);
		string input = buf.substr(j, i - j);

		if(cnt == 0){
			if(input == "ls") cmd_code = 0;
			else if(input == "cat") cmd_code = 1;
			else if(input == "exit") cmd_code = -1;
			else return -2;
		} else if (cnt == 1){
			if(cmd_code == 1) {
				name = input;
				set_file = true;
			}
			else if(cmd_code == 0){
				if(input[0] == '-'){
					if(input.size() == 1) return -2;
					for(int i = 1; i < input.size(); i++)
						if(input[i] != 'l') return -2;
					has_l = true;
				} else {
					name = input;
					set_file = true;
				}
			} else {
				return -2;
			}
		} else {
			if(cmd_code != 0) return -2;
			if(input[0] != '-' && set_file) return -2;
			if(input[0] == '-'){
				if(input.size() == 1) return -2;
				for(int i = 1; i < input.size(); i++)
					if(input[i] != 'l') return -2;
				has_l = true;
			} else {
				name = input;
				set_file = true;
			}
		}
		cnt++;

		for(; i < buf.size() && buf[i] == ' '; i++);
	}
	if(cmd_code == 1 && name == "") return -2;
	return cmd_code;
}

void load_img(FILE *fp){
	read_boot(fp);
	init_files();
	read_all_files(fp, 0, sectors_before_dir*SECTOR_SIZE, nums_of_dir_entry);
}

void read_boot(FILE* fp){
	fseek(fp, 11, SEEK_SET);
	fread(&boot, sizeof(boot), 1, fp);
	dir_entry_sectors = boot.num_of_dir_entry * ENTRY_SIZE / boot.bytes_per_sector;
	sectors_before_dir = 1 + boot.sector_per_fat * boot.num_of_fat;
	sectors_before_data = sectors_before_dir + dir_entry_sectors;
	nums_of_dir_entry = boot.num_of_dir_entry;
	bytes_of_fat = boot.sector_per_fat * boot.bytes_per_sector;
}

void init_files(){
	fn* root = new fn();
	root->name = "/";
	root->full_name = "/";
	root->attr = 0;
	root->father_pos = 0;
	root->fst_cluster = 0;
	files[files_idx++] = root;
	end_dir();
}

void read_all_files(FILE* fp, int father, int start, int ents){
	int begin = files_idx;
	files[father]->child_beign = files_idx;
	files[father]->direct_dirs = 0;
	files[father]->direct_files = 0;
	fseek(fp, start, SEEK_SET); // 定位到根目录区
	for(int i = 0; i < ents; i++){
		Entry ent;
		fread(&ent, sizeof(ent), 1, fp);

		if(ent.name[0] > 0 && (ent.attr == 0x10 || ent.attr == 0x20)){
			fn* node = new fn();
			string name = entry_name_toString(ent.name);

			node->name = name;
			if(father == 0)
				node->full_name = files[father]->full_name + name;
			else
				node->full_name = files[father]->full_name + "/" + name;
			node->fst_cluster = ent.fst_cluster;
			node->size = ent.size;
			int offset = (sectors_before_data + ent.fst_cluster - 2) * SECTOR_SIZE;
			node->data_beign = offset;
			node->child_beign = -1; // 如果不是目录的话就是-1
			node->father_pos = father;

			if(name == "."){
				node->attr = 2;
			} else if (name == ".."){
				node->attr = 3;
			} else{
				if(ent.attr == 0x10){ // 目录
					files[father]->direct_dirs++;
					node->attr = 0;
				} else if(ent.attr == 0x20){ // 文件
					files[father]->direct_files++;
					node->attr = 1;
				}
			}
			files[files_idx++] = node;
		}
	}
	end_dir();
	int end = files_idx;
	for(int i = begin; i < end; i++){
		if(files[i]->attr == 0){ // 如果是正常目录的话，继续读
			int cluster_no = files[i]->fst_cluster;
			read_all_files(fp, i, (sectors_before_data + cluster_no - 2) * SECTOR_SIZE, SECTOR_SIZE / ENTRY_SIZE);
		}
	}
}

void end_dir(){
	fn* split = new fn();
	split->attr = -1;
	files[files_idx++] = split;
}

fn* dirs[FILES_NUM];
int dir_idx = 0;

void deal_ls(){
	dir_idx = 0;
	if(name == "") name = "/";
	// 对输入的路径进行分割
	string split_names[SLASH_LEN]; int len = 0;
	int prev = 0;
	for(int i = 0; i < name.size(); i++){
		if(name[i] == '/'){
			if(len == 0 && i == 0){
				split_names[len++] = "/";
			} else{
				string s = name.substr(prev, i - prev);
				split_names[len++] = s;
			}
			prev = i + 1;
		} else if (i == name.size() - 1){
			string s = name.substr(prev, i - prev + 1);
			split_names[len++] = s;
		}
	}
	int cur_dir = 0;
	// cout << "==" << endl;
	for(int i = 0; i < len; i++){
		// cout << split_names[i] << endl;
		if (split_names[i] == ".") continue;
		else if (split_names[i] == ".."){
			cur_dir = files[cur_dir]->father_pos;
		} else {
			if(i == 0 && split_names[i] == "/") continue; // 根目录
			bool is_find = false;
			for(int j = files[cur_dir]->child_beign; files[j]->attr != -1; j++){
				if(files[j]->name == split_names[i]){
					is_find = true;
					cur_dir = j;
					break;
				}
			}
			if(!is_find){
				print(cant_find_dir_error, strlen(cant_find_dir_error));
				return;
			}
		}
	}
	// cout << "==" << endl;
	dirs[dir_idx ++] = files[cur_dir];
	for(int i = 0; i < dir_idx; i++){
		unflod_dir(dirs[i]);
	}
}

void unflod_dir(fn *dir){

	print_string(dir->full_name);

	if(dir->name != "/") print("/", 1);

	if(has_l) {
		print_blank();
		print_num(dir->direct_dirs);
		print_blank();
		print_num(dir->direct_files);
	}
	print(":", 1);
	print_newLine();

	for(int i = dir->child_beign; files[i]->attr != -1; i++){
		if(has_l){
			if(files[i]->attr == 0){
				dirs[dir_idx ++] = files[i];
				print_red(files[i]->name);
				print_blank();
				print_num(files[i]->direct_dirs);
				print_blank();
				print_num(files[i]->direct_files);
				print_newLine();
			}
			else if (files[i]->attr == 1){
				print_string(files[i]->name);
				print_blank();
				print_num(files[i]->size);
				print_newLine();
			}
			else if(files[i]->attr != -1){
				print_red(files[i]->name);
				print_newLine();
			}
		} else {
			if(files[i]->attr != -1){
				if(files[i]->attr == 1){
					print_string(files[i]->name);
					print_blank();
					print_blank();
				}
				else{
					if(files[i]->attr == 0) dirs[dir_idx ++] = files[i];
					print_red(files[i]->name);
					print_blank();
				}
			}
		}
	}
	if(!has_l) print_newLine();
}

w * fats;

void get_all_fat(FILE* fp){
    fseek(fp, boot.reserved_sectors * boot.bytes_per_sector, SEEK_SET);
    b* fat_bytes = new b[bytes_of_fat];
    fread(fat_bytes, bytes_of_fat, 1, fp);

	fats = new w[bytes_of_fat * 2 / 3];

    for(int i = 0, j = 0; j < bytes_of_fat; i+=2, j+=3){
        fats[i] =  (static_cast<w>(fat_bytes[j+1] & 0x0f) << 8) | fat_bytes[j];
        fats[i+1] =  static_cast<w>(fat_bytes[j+2] << 4) | ((fat_bytes[j+1] >> 4) & 0x0f);
    }

	delete fat_bytes;
}


void deal_cat(FILE* fp){
	fn * target = 0;
	bool is_find = false;
	for(int i = 0; i < files_idx; i++){
		if(name == files[i]->name || name == files[i]->full_name){
			if(files[i]->attr != 1){
				print(not_file_error, strlen(not_file_error));
				return;
			}
			is_find = true;
			target = files[i];
			break;
		}
	}
	if(!is_find) {
		print(cant_find_file_error, strlen(cant_find_file_error));
		return;
	}
	print_content(target, fp);
}

void print_content(fn* file, FILE* fp){
	get_all_fat(fp);
	for(int i = file->fst_cluster; i < 0xff7; i = fats[i]){
		fseek(fp, (sectors_before_data + i - 2) * SECTOR_SIZE, SEEK_SET);
		char content[SECTOR_SIZE];
		memset(content, 0, SECTOR_SIZE);
		fread(content, SECTOR_SIZE, 1, fp);
		print(content, strlen(content));
	}
}

int main(){

	FILE* fp = fopen(img_path, "rb");
	load_img(fp);

	bool set_exit = false;
	const char* prompt = "> ";
	while(true){
		print(prompt, 2);
		int code = deal_input();
		switch(code){
			case -2: print(cmd_error, strlen(cmd_error)); break;
			case -1: set_exit = true; break;
			case  0: deal_ls(); break;
			case  1: deal_cat(fp); break;
			default: break;
		}
		if(set_exit) break;
	}
	fclose(fp);
	return 0;
}
