#ifndef ORDER_BOOK_H
#define ORDER_BOOK_H

#include <pthread.h>

typedef enum { BUY, SELL } OrderType;

typedef struct {
    int order_id;
    OrderType type;
    double price;
    int quantity;
} Order;

#define MAX_ORDERS 10000

typedef struct {
    Order orders[MAX_ORDERS];
    int size;
    OrderType type; // BUY = Max Heap, SELL = Min Heap
} OrderHeap;

typedef struct {
    OrderHeap bids; 
    OrderHeap asks; 
    pthread_mutex_t lock;
    
    // Performance Metrics
    double total_volume;
    double total_value; 
    
    // Phase 7: Analytics & Indicators
    double ema10;
    double ema20;
    int trade_count;
} OrderBook;

void init_order_book(OrderBook *book);
void push_order(OrderBook *book, Order order);
void match_orders(OrderBook *book);
void cancel_order(OrderBook *book, int order_id, OrderType type);
void update_emas(OrderBook *book, double current_price);
double calculate_vwap(OrderBook *book);
double calculate_imbalance(OrderBook *book);

#endif