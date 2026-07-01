#include "order_book.h"
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#define NUM_ORDERS_PER_THREAD 150 

OrderBook book;
volatile int simulation_running = 1; 

// Simulated traders: I used sleep calls to force the threads to interleave
// so the mutex locks actually get tested under pressure.
void* simulate_buys(void* arg) {
    int thread_id = *(int*)arg;
    for (int i = 0; i < NUM_ORDERS_PER_THREAD; i++) {
        Order o;
        o.order_id = thread_id * 1000 + i;
        o.type = BUY;
        o.price = 98.0 + (rand() % 15); 
        o.quantity = 10 + (rand() % 90); 
        push_order(&book, o);
        usleep(30000); 
    }
    return NULL;
}

void* simulate_sells(void* arg) {
    int thread_id = *(int*)arg;
    for (int i = 0; i < NUM_ORDERS_PER_THREAD; i++) {
        Order o;
        o.order_id = thread_id * 1000 + i;
        o.type = SELL;
        o.price = 95.0 + (rand() % 15);  
        o.quantity = 10 + (rand() % 90); 
        push_order(&book, o);
        usleep(30000);
    }
    return NULL;
}

void* simulate_cancellations(void* arg) {
    usleep(50000); 
    int targets[] = {1010, 2025, 3030, 4015, 5020, 6040};
    OrderType target_types[] = {BUY, BUY, BUY, SELL, SELL, SELL};
    
    for (int i = 0; i < 6; i++) {
        cancel_order(&book, targets[i], target_types[i]);
        usleep(20000); 
    }
    return NULL;
}

// The UI thread - I used ANSI escape sequences to clear the screen 
// and re-draw the order book ladder.
void* render_dashboard(void* arg) {
    while (simulation_running) {
        printf("\033[H\033[J"); 
        
        pthread_mutex_lock(&book.lock); 
        
        double vwap = calculate_vwap(&book);
        
        printf("===============================================================\n");
        printf("           HFT MATCHING ENGINE - LIVE DASHBOARD                \n");
        printf("===============================================================\n");
        printf(" System Status : [ RUNNING ]\n");
        printf(" Total Volume  : %.0f units\n", book.total_volume);
        printf(" Market VWAP   : $%.4f\n", vwap);
        printf(" EMA10 / EMA20 : %.4f / %.4f\n", book.ema10, book.ema20);
        
        if (book.ema10 > book.ema20 && book.trade_count >= 20) {
            printf(" Trend Alert   : UPTREND (Bullish)\n");
        } else if (book.ema10 < book.ema20 && book.trade_count >= 20) {
            printf(" Trend Alert   : DOWNTREND (Bearish)\n");
        }
        
        printf("---------------------------------------------------------------\n");
        printf(" %-28s | %-28s\n", "BIDS", "ASKS");
        
        for (int i = 0; i < 5; i++) {
            char bid_str[35] = "", ask_str[35] = "";
            if (i < book.bids.size) sprintf(bid_str, "$%.2f @ %d", book.bids.orders[i].price, book.bids.orders[i].quantity);
            if (i < book.asks.size) sprintf(ask_str, "$%.2f @ %d", book.asks.orders[i].price, book.asks.orders[i].quantity);
            printf(" %-28s | %-28s\n", bid_str, ask_str);
        }
        
        pthread_mutex_unlock(&book.lock);
        usleep(100000); 
    }
    return NULL;
}

int main() {
    init_order_book(&book);
    srand(42); 

    pthread_t buy_threads[3], sell_threads[3], cancel_thread, dashboard_thread;
    int b_ids[3] = {1, 2, 3};
    int s_ids[3] = {4, 5, 6};

    pthread_create(&dashboard_thread, NULL, render_dashboard, NULL);
    usleep(10000);

    for (int i = 0; i < 3; i++) {
        pthread_create(&buy_threads[i], NULL, simulate_buys, &b_ids[i]);
        pthread_create(&sell_threads[i], NULL, simulate_sells, &s_ids[i]);
    }
    pthread_create(&cancel_thread, NULL, simulate_cancellations, NULL);

    for (int i = 0; i < 3; i++) {
        pthread_join(buy_threads[i], NULL);
        pthread_join(sell_threads[i], NULL);
    }
    pthread_join(cancel_thread, NULL);

    simulation_running = 0;
    pthread_join(dashboard_thread, NULL);

    printf("\n>>> SESSION CLOSED <<<\n");

    pthread_mutex_destroy(&book.lock);
    return 0;
}