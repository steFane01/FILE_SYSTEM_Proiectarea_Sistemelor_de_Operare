#ifndef OPERATIONS_H
#define OPERATIONS_H

void handle_list(int sock);
void handle_download(int sock);
void handle_upload(int sock);
void handle_delete(int sock);
void handle_move(int sock);
void handle_update(int sock);
void handle_search(int sock);

#endif // OPERATIONS_H
