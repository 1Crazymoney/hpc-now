/*
 * Copyright (C) 2022-present Shanghai HPC-NOW Technologies Co., Ltd.
 * This code is distributed under the license: MIT License
 * Originally written by Zhenrong WANG
 * mailto: zhenrongwang@live.com | wangzhenrong@hpc-now.com
 */

#ifndef PREREQ_CHECK_H
#define PREREQ_CHECK_H

int check_internet(void);
int check_internet_google(void);
int get_google_connectivity(void);
int file_validity_check(char* filename, int repair_flag, char* target_md5);
int check_current_user(void);
int install_bucket_clis(int silent_flag);
int repair_provider(char* plugin_root_path, char* cloud_name, char* provider_version, char* md5_exec, char* md5_zip, int force_repair_flag, char* seq_code);
int check_and_install_prerequisitions(int repair_flag);
int command_name_check(char* command_name_input, char* command_prompt, char* role_flag, char* cu_flag);
int command_parser(int argc, char** argv, char* command_name_prompt, char* workdir, char* cluster_name, char* user_name, char* cluster_role);

#endif