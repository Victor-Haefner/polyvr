#include "network.h"

Network::Network() {;}

int Network::add() {
    static int ID = -1;
    ID++;
    nodes[ID] = new node();
    return ID;
}

void Network::set(int n, void* data) {
    if (nodes.count(n) == 0) return;
    nodes[n]->data = data;
    return;
}

void* Network::get(int n) {
    if (nodes.count(n)) return nodes[n];
    return 0;
}

void Network::connect(int from, int to) {
    disconnect(from, to);
    nodes[from]->out[to] = nodes[to];
    nodes[to]->in[from] = nodes[from];
}

void Network::disconnect(int n1, int n2) {
    if (nodes[n1]->out.count(n2)) nodes[n1]->out.erase(n2);
    if (nodes[n2]->out.count(n1)) nodes[n2]->out.erase(n1);
    if (nodes[n1]->in.count(n2)) nodes[n1]->in.erase(n2);
    if (nodes[n2]->in.count(n1)) nodes[n2]->in.erase(n1);
}

void Network::isolate(int n) {
    nodes[n]->out.clear();
    nodes[n]->in.clear();
}
