/*
 * parallel modulation
 */
#include "pm.h"

int pmgetbtpimin(PM *d) {
    int i;
    struct timespec time_min;
    int min_id = -1, found = 0;
    for (i = 0; i < d->item_length; i++) {
        if (!found && d->item[i].output == PM_IDLE) {
            time_min = d->item[i].busy_time_total;
            min_id = i;
            found = 1;
        }
        if (found && d->item[i].output == PM_IDLE) {
            if (timespeccmp(&time_min, &d->item[i].busy_time_total, >)) {
                time_min = d->item[i].busy_time_total;
                min_id = i;
            }
        }
    }
    return min_id;
}

int pmgetbtpbmax(PM *d) {
    int i;
    struct timespec time_max;
    int max_id = -1, found = 0;
    for (i = 0; i < d->item_length; i++) {
        if (!found && d->item[i].output == PM_BUSY) {
            time_max = d->item[i].busy_time_total;
            max_id = i;
            found = 1;
        }
        if (found && d->item[i].output == PM_BUSY) {
            if (timespeccmp(&time_max, &d->item[i].busy_time_total, <)) {
                time_max = d->item[i].busy_time_total;
                max_id = i;
            }
        }
    }
    return max_id;
}

/*
 * min total busy time and idle time > idle_min
 * returns the best pin for turn on or -1 when there are no pins to turn on
 */
int pmgetbptn(PM *d) {
    int i;
    struct timespec time_r;
    int id = -1, found = 0;
    for (i = 0; i < d->item_length; i++) {
        if (!found && d->item[i].output == PM_IDLE && timespeccmp(&d->item[i].state_time_curr, &d->idle_min, >)) {
            time_r = d->item[i].busy_time_total;
            id = i;
            found = 1;
        }
        if (found && d->item[i].output == PM_IDLE && timespeccmp(&d->item[i].state_time_curr, &d->idle_min, >)) {
            if (timespeccmp(&time_r, &d->item[i].busy_time_total, >)) {
                time_r = d->item[i].busy_time_total;
                id = i;
            }
        }
    }
    if (id == -1) {//try to find pin with max state_time_curr
        found = 0;
        for (i = 0; i < d->item_length; i++) {
            if (!found && d->item[i].output == PM_IDLE) {
                time_r = d->item[i].state_time_curr;
                id = i;
                found = 1;
            }
            if (found && d->item[i].output == PM_IDLE) {
                if (timespeccmp(&time_r, &d->item[i].state_time_curr, <)) {
                    time_r = d->item[i].state_time_curr;
                    id = i;
                }
            }
        }
    }
    return id;
}

/*
 * max total busy time and busy time > busy_min
 * returns the best pin for turn off or -1 when there are no pins to turn off
 */
int pmgetbptf(PM *d) {
    int i;
    struct timespec time_r;
    int id = -1, found = 0;
    for (i = 0; i < d->item_length; i++) {
        if (!found && d->item[i].output == PM_BUSY && timespeccmp(&d->item[i].state_time_curr, &d->busy_min, >)) {
            time_r = d->item[i].busy_time_total;
            id = i;
            found = 1;
        }
        if (found && d->item[i].output == PM_BUSY && timespeccmp(&d->item[i].state_time_curr, &d->busy_min, >)) {
            if (timespeccmp(&time_r, &d->item[i].busy_time_total, <)) {
                time_r = d->item[i].busy_time_total;
                id = i;
            }
        }
    }
    if (id == -1) {//try to find pin with max state_time_curr
        found = 0;
        for (i = 0; i < d->item_length; i++) {
            if (!found && d->item[i].output == PM_BUSY) {
                time_r = d->item[i].state_time_curr;
                id = i;
                found = 1;
            }
            if (found && d->item[i].output == PM_BUSY) {
                if (timespeccmp(&time_r, &d->item[i].state_time_curr, <)) {
                    time_r = d->item[i].state_time_curr;
                    id = i;
                }
            }
        }
    }
    return id;
}

void pmctl(PM *d, int duty_cycle) {
    if (d->item_length <= 0) {
        return;
    }
    int i, min_id, max_id;
    //time monitor
    struct timespec now;
    clock_gettime(CLOCK_REALTIME_COARSE, &now);
    struct timespec time_new;
    struct timespec t_dif;
    if (!d->initialized) {
        for (i = 0; i < d->item_length; i++) {
            d->item[i].state_time_start = now;
            d->item[i].output = PM_IDLE;
            d->item[i].busy_time_total.tv_sec = 0;
            d->item[i].busy_time_total.tv_nsec = 0;
            d->item[i].state_time_curr.tv_sec = 0;
            d->item[i].state_time_curr.tv_nsec = 0;
        }
        d->busy_pin_num = 0;
        d->swap_time_start = now;
        d->initialized = 1;
    }
    for (i = 0; i < d->item_length; i++) {
        timespecsub(&now, &d->item[i].state_time_start, &time_new);
        timespecsub(&time_new, &d->item[i].state_time_curr, &t_dif);
        if (d->item[i].output == PM_BUSY) {
            timespecadd(&d->item[i].busy_time_total, &t_dif, &d->item[i].busy_time_total);
        }
        d->item[i].state_time_curr = time_new;
    }

    //SWAP controll (in order to keep busy time equal for all pins)
    //we will try to swap pins with max busy time and min busy time if busy pin is busy over duty_min and idle pin is idle over idle min
    timespecsub(&now, &d->swap_time_start, &t_dif);
    if (timespeccmp(&t_dif, &d->swap_delay, >)) {
        d->swap_time_start = now;
        min_id = pmgetbtpimin(d);
        max_id = pmgetbtpbmax(d);
        if (min_id >= 0 && max_id >= 0) {
            if (timespeccmp(&d->item[min_id].state_time_curr, &d->idle_min, >) && timespeccmp(&d->item[max_id].state_time_curr, &d->busy_min, >)) {
                int temp = d->item[min_id].output;
                d->item[min_id].output = d->item[max_id].output;
                d->item[max_id].output = temp;
                timespecclear(&d->item[max_id].state_time_curr);
                timespecclear(&d->item[min_id].state_time_curr);
                d->item[min_id].state_time_start = now;
                d->item[max_id].state_time_start = now;
            }
        }
    }
    //duty_cycle normalization
    if (duty_cycle < 0) {
        duty_cycle = 0;
    } else if (duty_cycle > PM_RSL) {
        duty_cycle = PM_RSL;
    }
    //POWER controll
    //if required, we will turn off pins with max busy time first (and current busy time > busy_min state time if possible) 
    //if required, we will turn on pins with min busy time first (and current idle time > idle_min state time if possible) 
    int n = ((float) duty_cycle / (float) PM_RSL) * (float) d->item_length;
    if (n > d->busy_pin_num) {//we need to turn on more pins
        int dif = n - d->busy_pin_num; //number of pins to turn on
        int i;
        for (i = 0; i < dif; i++) {
            min_id = pmgetbptn(d);
            if (min_id >= 0) {
                d->item[min_id].output = PM_BUSY;
                timespecclear(&d->item[min_id].state_time_curr);
                d->item[min_id].state_time_start = now;
                d->busy_pin_num++;
            }
        }
    } else if (n < d->busy_pin_num) {//we need to turn off more pins
        int dif = d->busy_pin_num - n; //number of pins to turn off
        int i;
        for (i = 0; i < dif; i++) {
            max_id = pmgetbptf(d);
            // printf("turn off is %d\n", max_id);
            if (max_id >= 0) {
                d->item[max_id].output = PM_IDLE;
                timespecclear(&d->item[max_id].state_time_curr);
                d->item[max_id].state_time_start = now;
                d->busy_pin_num--;
            }
        }
    }
}

