/**************************************************************************************************
 * Files module
 *
 * Author: Razvan Madalin MATEI <matei.rm94@gmail.com
 * Date last modified: April 2015
 *************************************************************************************************/

#ifndef _FILES_H
#define _FILES_H

#ifdef FILES

/**
 * Parse files command
 */
void files(const char *from, const char *to, int error, xmpp_stanza_t *stanza,
           xmpp_conn_t *const conn, void *const userdata);

int wfiles_getattr(const char *path, struct stat *stbuf);

int wfiles_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                  off_t offset, struct fuse_file_info *fi);

#endif /* FILES */

#endif /* _FILES_H */
