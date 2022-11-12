#include <iostream>
#include <string.h>
#include <vector>
#include <stdio.h>

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
	string name;
	string full_name;
	int attr;
} fn;

// =========================================================================================
static const char* img_path = "../../a.img";
Boot boot;
bool contains_l = false;
string name; // ls的目录名 cat的文件名
int dir_entry_sectors; // 根目录的扇区数
int sectors_before_dir; // 根目录前的扇区数
int sectors_before_data; // 数据区前的扇区数
int bytes_of_fat; // fat的字节数

// ========================================================================================
extern "C"{
	void print(char *s, int len);
}
int deal_input();
void load_img();
void deal_ls();
void deal_cat();
void seek_dir_in_data(FILE* fp, int cluster);

// ========================================================================================

// ============ 工具函数 =========

string deal_entry_name(const char *s){
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
    string name = deal_entry_name(s2);
    return s1 == name;
}


// ============ 核心函数 =========
void load_img(FILE* fp){
	fseek(fp, 11, SEEK_SET);
	fread(&boot, sizeof(boot), 1, fp);
	dir_entry_sectors = boot.num_of_dir_entry * ENTRY_SIZE / boot.bytes_per_sector;
	sectors_before_dir = 1 + boot.sector_per_fat * boot.num_of_fat;
	sectors_before_data = sectors_before_dir + dir_entry_sectors;
    bytes_of_fat = boot.sector_per_fat * boot.bytes_per_sector;
}

// 1. 到根目录区查找文件/目录的对应项
// 2. 到FAT中获取目标文件的起始簇号和文件大小
// 3. 根据FAT表的顺序来读取数据

void print_all_entries(FILE* fp){
    fseek(fp, sectors_before_dir * SECTOR_SIZE, SEEK_SET);
    int entry_nums = boot.num_of_dir_entry;
    for(int i = 0; i < entry_nums; i++){
        Entry ent;
        fread(&ent, sizeof(ent), 1, fp);
        if(ent.name[0] > 0){
            string name = deal_entry_name(ent.name);
            if( ent.attr == 0x10 ) // 目录
                cout << "\033[31m" << name << "\033[0m" << "  ";
            if( ent.attr == 0x20 ) // 文件
                cout << name << "  ";
        }
    }
    cout << endl;
}

Entry find_entry_by_id(FILE* fp, int no){
	fseek(fp, sectors_before_dir * SECTOR_SIZE, SEEK_SET);
	fseek(fp, no * ENTRY_SIZE, SEEK_CUR);
	Entry ent;
	fread(&ent, sizeof(ent), 1, fp);
	return ent;
}

Entry find_entry_by_name(FILE* fp, const string name){
	fseek(fp, sectors_before_dir * SECTOR_SIZE, SEEK_SET); // 定位到根目录区
	int entry_nums = boot.num_of_dir_entry;
	Entry ent;
	for(int i = 0; i < entry_nums; i++){
		fread(&ent, sizeof(ent), 1, fp);
        if((ent.attr == 0x10 || ent.attr == 0x20) && ent.name[0] > 0)
		    if( cmp_name(name, ent.name) ) 
                break;
	}
	return ent;
}

/**
 * 有没有 -l
 *      没有 -l 只需要列出 目录和文件
 *      有 -l 对于文件 需要列出 size 对于目录需要列出直接子目录数和直接子文件数
 * 有没有指定 dir
 */

vector<w> get_all_fat(FILE* fp){
    // 移动到FAT1开始位置
    fseek(fp, boot.reserved_sectors * boot.bytes_per_sector, SEEK_SET);
    b* fats = new b[bytes_of_fat];
    fread(fats, bytes_of_fat, 1, fp);
    vector<w> res(bytes_of_fat);
    // (XX X)(X XX)
    for(int i = 0, j = 0; i < bytes_of_fat; i+=2, j+=3){
        res[i] =  (static_cast<w>(fats[j+1] & 0x0f) << 8) | fats[j];
        res[i+1] =  static_cast<w>(fats[j+2] << 4) | ((fats[j+1] >> 4) & 0x0f);
    }
    return res;
}

// 实现 ls
void preprocess(){
    contains_l = false;
    name = "NJU";
}
void deal_ls(FILE* fp){
    // preprocess(); // 代替处理输入
    // cout << "/" << name << ":" << endl;
    string file = "NJU";
    Entry ent = find_entry_by_name(fp, file); 
    cout << ent.name << " " << ent.fst_cluster << " " << ent.size << endl;
    seek_dir_in_data(fp, ent.fst_cluster);
    // vector<w> fat_values = get_all_fat(fp);
    // for(int i = 0; i <= ent.fst_cluster; i++){
    //     cout << fat_values[i] << endl;
    // }
}

void seek_dir_in_data(FILE* fp, int cluster){
    fseek(fp, sectors_before_data * SECTOR_SIZE, SEEK_SET);
    fseek(fp, (cluster - 2) * SECTOR_SIZE, SEEK_CUR);
    Entry ent;
    for(int i = 0; i < 16; i++){
        fread(&ent, sizeof(ent), 1, fp);
        if(ent.name[0] > 0)
            printf("%s, %u, %u, %u\n", ent.name, ent.attr, ent.fst_cluster, ent.size);
    }
}

// ========================================================================================
int main(){

	FILE* fp = fopen(img_path, "rb");
	load_img(fp);

    print_all_entries(fp);
    deal_ls(fp);

    fclose(fp);
	return 0;
}
