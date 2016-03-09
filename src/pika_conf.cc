#include "sys/stat.h"
#include "base_conf.h"
#include "pika_conf.h"
#include "glog/logging.h"

#include <fstream>
#include <string>


PikaConf::PikaConf(const char* path) :
    BaseConf(path)
{
    strcpy(conf_path_, path);

    getConfInt("port", &port_);
    getConfInt("thread_num", &thread_num_);
    getConfInt("slave_thread_num", &slave_thread_num_);
    getConfStr("log_path", log_path_);
    getConfInt("log_level", &log_level_);
    getConfStr("db_path", db_path_);
    getConfInt("write_buffer_size", &write_buffer_size_);
    getConfInt("timeout", &timeout_);
    getConfStr("requirepass", requirepass_);
    getConfStr("userpass", userpass_);
    // Set User Blacklist
    char user_blacklist[PIKA_CONF_MAX_NUM];
    getConfStr("userblacklist", user_blacklist);
    SetUserBlackList(std::string(user_blacklist));

    getConfStr("dump_prefix", dump_prefix_);
    getConfStr("dump_path", dump_path_);
    getConfStr("master_db_sync_path", master_db_sync_path_);
    getConfStr("slave_db_sync_path", slave_db_sync_path_);
    getConfStr("compression", compression_);
    getConfStr("username", username_);
    getConfStr("password", password_);

    getConfStr("pidfile", pidfile_);

    getConfInt("maxconnection", &maxconnection_);
    getConfInt("target_file_size_base", &target_file_size_base_);
    getConfInt("expire_logs_days", &expire_logs_days_);
    getConfInt("expire_logs_nums", &expire_logs_nums_);
    getConfInt("root_connection_num", &root_connection_num_);
    getConfInt("slowlog_log_slower_than", &slowlog_slower_than_);
    getConfInt("binlog_file_size", &binlog_file_size_);
    getConfInt("db_sync_speed", &db_sync_speed_);
    getConfBool("slave-read-only", &readonly_);

    if (thread_num_ <= 0) {
        thread_num_ = 16;
    }
    if (slave_thread_num_ <= 0) {
        slave_thread_num_ = 7;
    }
    if (write_buffer_size_ <= 0 ) {
        write_buffer_size_ = 4194304; // 40M
    }
    if (timeout_ <= 0) {
        timeout_ = 60; // 60s
    }
    if (maxconnection_ <= 0) {
        maxconnection_ = 20000;
    }
    if (target_file_size_base_ <= 0) {
        target_file_size_base_ = 1048576; // 10M
    }
    if (expire_logs_days_ <= 0 ) {
        expire_logs_days_ = 1;
    }
    if (expire_logs_nums_ <= 10 ) {
        expire_logs_nums_ = 10;
    }
    if (root_connection_num_ < 0) {
        root_connection_num_ = 0;
    }
    if (db_sync_speed_ < 0 || db_sync_speed_ > 125) {
        db_sync_speed_ = 125;
    }
    char str[PIKA_WORD_SIZE];
    getConfStr("daemonize", str);

    if (strcmp(str, "yes") == 0) {
        daemonize_ = true;
    } else {
        daemonize_ = false;
    }

    if (binlog_file_size_ < 1024 || static_cast<int64_t>(binlog_file_size_) > (1024LL * 1024 * 1024 * 2)) {
      binlog_file_size_ = 100 * 1024 * 1024;    // 100M
    }

    pthread_rwlock_init(&rwlock_, NULL);
}

PikaConf::~PikaConf() {
    pthread_rwlock_destroy(&rwlock_);
    stop_rsync(slave_db_sync_path_);
    /*
    char self_pid_str[10];
    snprintf(self_pid_str, sizeof(self_pid_str), "%d", getpid());
    std::string rsync_pid_file_path;
    if (slave_db_sync_path_[strlen(slave_db_sync_path_)] == '/') {
        rsync_pid_file_path.assign(slave_db_sync_path_, strlen(slave_db_sync_path_)-1);
    } else {
        rsync_pid_file_path.assign(slave_db_sync_path_);
    }
    if (rsync_pid_file_path == ".") {
        char buf[100];
        getcwd(buf, sizeof(buf));
        rsync_pid_file_path.assign(buf);
    }
    if (!rsync_pid_file_path.empty()) {
        size_t last_slash_pos = rsync_pid_file_path.find_last_of("/");
        rsync_pid_file_path = rsync_pid_file_path.substr(0, last_slash_pos);
    }
    rsync_pid_file_path.append("/pika_rsync_");
    rsync_pid_file_path.append(self_pid_str);
    rsync_pid_file_path.append(".pid");
    if (access(rsync_pid_file_path.c_str(), F_OK) == 0) {
        stop_rsync(slave_db_sync_path_);
    }
    */
}

int PikaConf::ConfigRewrite() {
    std::string conf_file_new = std::string(conf_path_, strlen(conf_path_)) + "_new";
    std::ofstream cfn(conf_file_new.c_str());
    if (!cfn) {
        LOG(WARNING) << "open new conf file error!";
        return -1;
    }
    cfn << "# Pika port\nport : " << port() << std::endl;
    cfn << "# Thread Number\nthread_num : " << thread_num() << std::endl;
    cfn << "# Slave Thread Number\nslave_thread_num : " << slave_thread_num() << std::endl;
    cfn << "# Pika log path\nlog_path : " << log_path() << std::endl;
    cfn << "# Pika glog level\nlog_level : " << log_level() << std::endl;
    cfn << "# Pika db path\ndb_path : " << db_path() << std::endl;
    cfn << "# master db_sync_path\nmaster_db_sync_path : " << master_db_sync_path() << std::endl;
    cfn << "# slave db_sync_path\nslave_db_sync_path : " << slave_db_sync_path() << std::endl;
    cfn << "# write_buffer_size\nwrite_buffer_size : " << write_buffer_size() << std::endl;
    cfn << "# Pika timeout\ntimeout : " << timeout() << std::endl;
    cfn << "# Requirepass\nrequirepass : " << requirepass() << std::endl;
    cfn << "# Requirepass\nuserpass : " << userpass() << std::endl;
    cfn << "# Requirepass\nuserblacklist : " << suser_blacklist().c_str() << std::endl;
    cfn << "# Dump Prefix\ndump_prefix : " << dump_prefix() << std::endl;
    cfn << "# daemonize  [yes | no]\n#daemonize : yes" << std::endl;
    cfn << "# Dump Path\ndump_path : " << dump_path() << std::endl;
    cfn << "# pidfile Path\npidfile : " << pidfile() << std::endl;
    cfn << "# Max Connection\nmaxconnection : " << maxconnection() << std::endl;
    cfn << "# the per file size of sst to compact, defalut is 2M\ntarget_file_size_base : " << target_file_size_base() << std::endl;
    cfn << "# Expire_logs_days\nexpire_logs_days : " << expire_logs_days() << std::endl;
    cfn << "# Expire_logs_nums\nexpire_logs_nums : " << expire_logs_nums() << std::endl;
    cfn << "# Root_connection_num\nroot_connection_num : " << root_connection_num() << std::endl;
    cfn << "# Slowlog_log_slower_than\nslowlog_log_slower_than : " << slowlog_slower_than() << std::endl;
    cfn << "# readonly(on/off)\nslave-read-only : " << readonly() << std::endl;
    cfn << "# username for db_sync\nusername : " << username() << std::endl;
    cfn << "# password for db_sync\npassword : " << password() << std::endl;
    cfn << "# db sync speed(MB)\n db_sync_speed : " << db_sync_speed() << std::endl;
    cfn << "\n###################\n## Critical Settings\n###################" << std::endl;
    cfn << "# binlog file size: default is 100M,  limited in [1K, 2G]\nbinlog_file_size : " << binlog_file_size() << std::endl;
    cfn << "# Compression\ncompression : " << compression() << std::endl;
    cfn.close();
    {
        RWLock l(&rwlock_, true);
        if (rename(conf_file_new.c_str(), conf_path_)) {
            LOG(WARNING) << "rename new conf file name to origin conf file error!";
            return -2;
        }
    }
    return 0;
}