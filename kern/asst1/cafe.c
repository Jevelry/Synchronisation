/* This file will contain your solution. Modify it as you wish. */
#include <types.h>
#include <lib.h>
#include <synch.h>
#include "cafe.h"

/* Some variables shared between threads */

static unsigned int ticket_counter; /* the next customer's ticket */
static unsigned int next_serving;   /* the barista's next ticket to serve */
static unsigned current_customers;  /* customers remaining in cafe */

// CUSTOM VARIABLES
unsigned int customer_tickets[NUM_CUSTOMERS];
unsigned int barista_tickets[NUM_BARISTAS];

struct cv * customer_cv[NUM_CUSTOMERS];
struct cv * barista_cv[NUM_BARISTAS];

unsigned int serving;

struct lock *ticket_lock;
struct lock *next_serving_lock;
struct lock *leave_cafe_lock;
struct lock *main;

//CUSTOM FUNCTION
int is_ticket_grabbed(unsigned int ticket);
int is_ticket_announced(unsigned int ticket);

/*
 * get_ticket: generates an ticket number for a customers next order.
 */

unsigned int get_ticket(void)
{
        unsigned int t;
        lock_acquire(ticket_lock);
        ticket_counter = ticket_counter + 1;
        t = ticket_counter;
        lock_release(ticket_lock);
        return t;
}

/*
 * next_ticket_to_serve: generates the next ticket number for a
 * barista for that barista to serve next.
 */

unsigned int next_ticket_to_serve(void)
{

        unsigned int t;
        lock_acquire(next_serving_lock);
        next_serving = next_serving + 1;
        t = next_serving;
        lock_release(next_serving_lock);
        return t;
}

/*
 * leave_cafe: a function called by a customer thread when the
 * specific thread leaves the cafe.
 */

void leave_cafe(unsigned long customer_num)
{
        (void)customer_num;
        lock_acquire(leave_cafe_lock);
        current_customers = current_customers - 1;
        lock_release(leave_cafe_lock);
}


/*
 * wait_to_order() and announce_serving_ticket() work together to
 * achieve the following:
 *
 * A customer thread calling wait_to_order will block on a synch primitive
 * until announce_serving_ticket is called with the matching ticket.
 *
 * A barista thread calling announce_serving_ticket will block on a synch
 * primitive until the corresponding ticket is waited on, OR there are
 * no customers left in the cafe.
 *
 * wait_to_order returns the number of the barista that will serve
 * the calling customer thread.
 *
 * announce_serving_ticket returns the number of the customer that the
 * calling barista thread will serve.
 */

unsigned long wait_to_order(unsigned long customer_number, unsigned int ticket)
{
        unsigned long barista_number = 255;

        (void) customer_number;
        (void) ticket;

        lock_acquire(main);

        customer_tickets[customer_number] = ticket;
        

        while(is_ticket_announced(ticket) == -1) {
                cv_wait(customer_cv[customer_number], main);
        }

        barista_number = is_ticket_announced(ticket);
        cv_signal(barista_cv[barista_number], main);
        lock_release(main);

        return barista_number;
}

unsigned long announce_serving_ticket(unsigned long barista_number, unsigned int serving)
{
        unsigned long cust = 256;

        (void) barista_number;
        (void) serving;

        lock_acquire(main);
        barista_tickets[barista_number] = serving;

        while(is_ticket_grabbed(serving) == -1 || current_customers == 0) {
                cv_wait(barista_cv[barista_number], main);
        }

        
        cust = is_ticket_grabbed(serving);
        cv_signal(customer_cv[cust], main);
        lock_release(main);

        return cust;
}
// function to check if barista has stored ticket inside barista tickets
int is_ticket_announced(unsigned int ticket) {
        for (int i = 0; i < NUM_BARISTAS; i++) {
                if (barista_tickets[i] == ticket) {
                        return i;
                }
        }
        return -1;
}
// function to check if barista has stored ticket inside customer tickets

int is_ticket_grabbed(unsigned int ticket) {
        for (int i = 0; i < NUM_CUSTOMERS; i++) {
                if (customer_tickets[i] == ticket) {
                        return i;
                }
        }
        return -1;
}


/* 
 * cafe_startup: A function to allocate and/or intitialise any memory
 * or synchronisation primitives that are needed prior to the
 * customers and baristas arriving in the cafe.
 */
void cafe_startup(void)
{

        ticket_counter = 0;
        next_serving = 0;
        current_customers = NUM_CUSTOMERS;



        for (int i = 0; i < NUM_CUSTOMERS; i++){
                customer_cv[i] = cv_create("customer");
        }

        for (int i = 0; i < NUM_BARISTAS; i++){
                barista_cv[i] = cv_create("barista");
        }

        ticket_lock = lock_create("ticket_lock");
        next_serving_lock = lock_create("next_serving_lock");
        leave_cafe_lock = lock_create("leave_cafe_lock");
        main = lock_create("main");

}   

/*
 * cafe_shutdown: A function called after baristas and customers have
 * exited to de-allocate any memory or synchronisation
 * primitives. Anything allocated during startup should be
 * de-allocated after calling this function.
 */

void cafe_shutdown(void)
{
        lock_destroy(ticket_lock);
        lock_destroy(next_serving_lock);
        lock_destroy(leave_cafe_lock);
        lock_destroy(main);
        for (int i = 0; i < NUM_CUSTOMERS; i++){
                cv_destroy(customer_cv[i]);
        }

        for (int i = 0; i < NUM_BARISTAS; i++){
                cv_destroy(barista_cv[i]);
        }
        for (int i = 0; i < NUM_CUSTOMERS; i++){
                customer_tickets[i] = 0;
        }
        for (int i = 0; i < NUM_BARISTAS; i++){
                barista_tickets[i] = 0;
        }
}
                              
