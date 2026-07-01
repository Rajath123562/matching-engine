\# High-Performance Matching Engine in C



I built this project to challenge myself with low-latency systems programming. Instead of using high-level abstractions, I wanted to understand how exchanges \*actually\* move data—so I built this engine from the ground up using C and Pthreads.



\## Why I Built This

I’ve always been interested in how financial systems handle thousands of orders per second. This project was my way of diving into:

\- \*\*Concurrency\*\*: Managing multiple market participants (threads) without data races.

\- \*\*Data Structures\*\*: Using heaps to keep order matching at O(log N) efficiency.

\- \*\*Real-time Analytics\*\*: Implementing EMA (Exponential Moving Average) crossovers so the engine can "think" about market trends while it processes trades.



\## What's Under the Hood

This isn't just a simple script; it’s a full-stack backend written in C:

\- \*\*Engine\*\*: Uses a custom Min/Max heap implementation for order matching.

\- \*\*Thread Safety\*\*: Uses mutex locks to handle simultaneous buy/sell orders.

\- \*\*Analytics\*\*: I added a module to calculate VWAP and EMA(10/20) on the fly, including logic to trigger alerts when trends flip.

\- \*\*UI\*\*: A live, Bloomberg-style dashboard that I rendered using ANSI escape sequences. It refreshes 10 times a second and shows the order book depth in real-time.



\## How to Get It Running

I developed this on Windows using GCC. You’ll need a C compiler installed.



1\. \*\*Compile\*\*:

&#x20;  ```bash

&#x20;  gcc -O3 order\_book.c main.c -o exchange\_engine.exe -lpthread

