#include "order_book.h"
#include <stdio.h>
#include <stdlib.h>

// I used a standard heap structure here. It took some trial and error 
// to make sure the heapify logic didn't go out of bounds!
void init_heap(OrderHeap *heap, OrderType type) {
    heap->size = 0;
    heap->type = type;
}

void swap_orders(Order *a, Order *b) {
    Order temp = *a;
    *a = *b;
    *b = temp;
}

void heap_push(OrderHeap *heap, Order order) {
    if (heap->size >= MAX_ORDERS) return;
    
    int i = heap->size++;
    heap->orders[i] = order;
    
    while (i > 0) {
        int parent = (i - 1) / 2;
        if (heap->type == BUY) {
            if (heap->orders[i].price > heap->orders[parent].price) {
                swap_orders(&heap->orders[i], &heap->orders[parent]);
                i = parent;
            } else break;
        } else {
            if (heap->orders[i].price < heap->orders[parent].price) {
                swap_orders(&heap->orders[i], &heap->orders[parent]);
                i = parent;
            } else break;
        }
    }
}

void heapify(OrderHeap *heap, int idx) {
    int target = idx;
    int left = 2 * idx + 1;
    int right = 2 * idx + 2;
    
    if (heap->type == BUY) {
        if (left < heap->size && heap->orders[left].price > heap->orders[target].price)
            target = left;
        if (right < heap->size && heap->orders[right].price > heap->orders[target].price)
            target = right;
    } else {
        if (left < heap->size && heap->orders[left].price < heap->orders[target].price)
            target = left;
        if (right < heap->size && heap->orders[right].price < heap->orders[target].price)
            target = right;
    }
    
    if (target != idx) {
        swap_orders(&heap->orders[idx], &heap->orders[target]);
        heapify(heap, target);
    }
}

Order heap_pop(OrderHeap *heap) {
    Order top = heap->orders[0];
    heap->orders[0] = heap->orders[--heap->size];
    heapify(heap, 0);
    return top;
}

void init_order_book(OrderBook *book) {
    init_heap(&book->bids, BUY);
    init_heap(&book->asks, SELL);
    pthread_mutex_init(&book->lock, NULL);
    book->total_volume = 0;
    book->total_value = 0;
    book->ema10 = 0.0;
    book->ema20 = 0.0;
    book->trade_count = 0;
}

// Updating EMAs as trades happen. 
// Standard weighting (2 / N+1) formula.
void update_emas(OrderBook *book, double current_price) {
    if (book->trade_count < 10) {
        book->ema10 = current_price;
    } else {
        double k10 = 2.0 / (10.0 + 1.0);
        book->ema10 = (current_price - book->ema10) * k10 + book->ema10;
    }

    if (book->trade_count < 20) {
        book->ema20 = current_price;
    } else {
        double k20 = 2.0 / (20.0 + 1.0);
        book->ema20 = (current_price - book->ema20) * k20 + book->ema20;
    }
    book->trade_count++;
}

void match_orders(OrderBook *book) {
    while (book->bids.size > 0 && book->asks.size > 0) {
        Order *best_bid = &book->bids.orders[0];
        Order *best_ask = &book->asks.orders[0];
        
        if (best_bid->price >= best_ask->price) {
            double match_price = best_ask->price; 
            int match_qty = (best_bid->quantity < best_ask->quantity) ? best_bid->quantity : best_ask->quantity;
            
            book->total_volume += match_qty;
            book->total_value += (match_price * match_qty);
            
            update_emas(book, match_price);
            
            best_bid->quantity -= match_qty;
            best_ask->quantity -= match_qty;
            
            if (best_bid->quantity == 0) heap_pop(&book->bids);
            if (best_ask->quantity == 0) heap_pop(&book->asks);
        } else {
            break;
        }
    }
}

void push_order(OrderBook *book, Order order) {
    // This lock is crucial for thread safety!
    pthread_mutex_lock(&book->lock);
    if (order.type == BUY) {
        heap_push(&book->bids, order);
    } else {
        heap_push(&book->asks, order);
    }
    match_orders(book);
    pthread_mutex_unlock(&book->lock);
}

void cancel_order(OrderBook *book, int order_id, OrderType type) {
    pthread_mutex_lock(&book->lock);
    
    OrderHeap *heap = (type == BUY) ? &book->bids : &book->asks;
    int target_idx = -1;

    // Linear search is O(N), but it's fine for my simulation size.
    for (int i = 0; i < heap->size; i++) {
        if (heap->orders[i].order_id == order_id) {
            target_idx = i;
            break;
        }
    }

    if (target_idx != -1) {
        heap->orders[target_idx] = heap->orders[heap->size - 1]; 
        heap->size--;
        heapify(heap, target_idx); 
    }

    pthread_mutex_unlock(&book->lock);
}

double calculate_vwap(OrderBook *book) {
    if (book->total_volume == 0) return 0.0;
    return book->total_value / book->total_volume;
}

double calculate_imbalance(OrderBook *book) {
    int bid_vol = 0;
    int ask_vol = 0;
    for (int i = 0; i < book->bids.size; i++) bid_vol += book->bids.orders[i].quantity;
    for (int i = 0; i < book->asks.size; i++) ask_vol += book->asks.orders[i].quantity;
    
    if ((bid_vol + ask_vol) == 0) return 0.0;
    return (double)bid_vol / (bid_vol + ask_vol);
}