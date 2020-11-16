#ifndef STOCK_H
#define STOCK_H

struct stock
{
    int n;
    char *livres;
    int *disp;
};

void init_stock(int n, char *argv[], struct stock *st);
void free_stock(struct stock *st);
int reserver_livre(const char *titre, struct stock *st);


#endif