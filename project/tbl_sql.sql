-- �����ļ���
CREATE TABLE `tbl_file` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `file_sha1` char(40) NOT NULL DEFAULT '' COMMENT '�ļ�hash',
  `file_name` varchar(256) NOT NULL DEFAULT '' COMMENT '�ļ���',
  `file_size` bigint(20) DEFAULT '0' COMMENT '�ļ���С',
  `file_addr` varchar(1024) NOT NULL DEFAULT '' COMMENT '�ļ��洢λ��',
  `create_at` datetime default NOW() COMMENT '��������',
  `update_at` datetime default NOW() on update current_timestamp() COMMENT '��������',
  `status` int(11) NOT NULL DEFAULT '0' COMMENT '״̬(����/����/��ɾ����״̬)',
  `ext1` int(11) DEFAULT '0' COMMENT '�����ֶ�1',
  `ext2` text COMMENT '�����ֶ�2',
  PRIMARY KEY (`id`),
  UNIQUE KEY `idx_file_hash` (`file_sha1`),
  KEY `idx_status` (`status`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

-- �����û���
CREATE TABLE `tbl_user` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `user_name` varchar(64) NOT NULL DEFAULT '' COMMENT '�û���',
  `user_pwd` varchar(256) NOT NULL DEFAULT '' COMMENT '�û�encoded����',
  `email` varchar(64) DEFAULT '' COMMENT '����',
  `phone` varchar(128) DEFAULT '' COMMENT '�ֻ���',
  `email_validated` tinyint(1) DEFAULT 0 COMMENT '�����Ƿ�����֤',
  `phone_validated` tinyint(1) DEFAULT 0 COMMENT '�ֻ����Ƿ�����֤',
  `signup_at` datetime DEFAULT CURRENT_TIMESTAMP COMMENT 'ע������',
  `last_active` datetime DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP COMMENT '����Ծʱ���',
  `profile` text COMMENT '�û�����',
  `status` int(11) NOT NULL DEFAULT '0' COMMENT '�˻�״̬(����/����/����/���ɾ����)',
  PRIMARY KEY (`id`),
  UNIQUE KEY `idx_username` (`user_name`),
  KEY `idx_status` (`status`)
) ENGINE=InnoDB AUTO_INCREMENT=5 DEFAULT CHARSET=utf8mb4;

-- �����û�token��
CREATE TABLE `tbl_user_token` (
    `id` int(11) NOT NULL AUTO_INCREMENT,
  `user_name` varchar(64) NOT NULL DEFAULT '' COMMENT '�û���',
  `user_token` char(40) NOT NULL DEFAULT '' COMMENT '�û���¼token',
    PRIMARY KEY (`id`),
  UNIQUE KEY `idx_username` (`user_name`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- �����û��ļ���
CREATE TABLE `tbl_user_file` (
  `id` int(11) NOT NULL PRIMARY KEY AUTO_INCREMENT,
  `user_name` varchar(64) NOT NULL,
  `file_sha1` varchar(64) NOT NULL DEFAULT '' COMMENT '�ļ�hash',
  `file_size` bigint(20) DEFAULT '0' COMMENT '�ļ���С',
  `file_name` varchar(256) NOT NULL DEFAULT '' COMMENT '�ļ���',
  `upload_at` datetime DEFAULT CURRENT_TIMESTAMP COMMENT '�ϴ�ʱ��',
  `last_update` datetime DEFAULT CURRENT_TIMESTAMP 
          ON UPDATE CURRENT_TIMESTAMP COMMENT '����޸�ʱ��',
  `status` int(11) NOT NULL DEFAULT '0' COMMENT '�ļ�״̬(0����1��ɾ��2����)',
  UNIQUE KEY `idx_user_file` (`user_name`, `file_sha1`),
  KEY `idx_status` (`status`),
  KEY `idx_user_id` (`user_name`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

truncate tbl_file;truncate tbl_user;truncate tbl_user_file;truncate tbl_user_token;