// 
// config.hh     : Common defines + other config parameters
// authors       : Chalermek Intanagonwiwat and Fabio Silva
//
// Copyright (C) 2000-2001 by the Unversity of Southern California
// $Id: config.hh,v 1.5 2002/03/20 22:49:40 haldar Exp $
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License,
// version 2, as published by the Free Software Foundation.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License along
// with this program; if not, write to the Free Software Foundation, Inc.,
// 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
//
//

// Software information
#define PROGRAM "Diffusion 3.0.7"
#define RELEASE "Gear Alpha Release"
#define DEFAULT_CONFIG_FILE "config.txt"

// Timers
#define INTEREST_TIMER         1
#define ENERGY_TIMER           2
#define GRADIENT_TIMER         3
#define SUBSCRIPTION_TIMER     4
#define NEIGHBORS_TIMER        5
#define FILTER_TIMER           6
#define FILTER_KEEPALIVE_TIMER 7
#define APPLICATION_TIMER      8
#define REINFORCEMENT_TIMER    9
#define MESSAGE_SEND_TIMER     10
#define STOP_TIMER             11
#define PACKET_RECEIVED        12

// Debug information
#define DEBUG_NEVER            11
#define DEBUG_LOTS_DETAILS     10
#define DEBUG_MORE_DETAILS      8
#define DEBUG_DETAILS           6
#define DEBUG_SOME_DETAILS      4
#define DEBUG_NO_DETAILS        3
#define DEBUG_IMPORTANT         2
#define DEBUG_ALWAYS            1

#ifdef NS_DIFFUSION
#define DEBUG_DEFAULT           0
#else
#define DEBUG_DEFAULT           1
#endif // NS_DIFFUSION

#ifdef BBN_LOGGER
#define LOGGER_BUFFER_SIZE 512
#define LOGGER_CONFIG_FILE "/sensit/Logger.ISI"
#endif // BBN_LOGGER

// Configurable parameters start here

// Change the following parameters to set how often
// interest messages are sent.
#define INTEREST_DELAY          25000        // (msec) bw sends
#define INTEREST_JITTER          5000        // (msec) jitter

// This parameter specifies how often an exploratory message is allowed
// to go to the network. It's used to establish a reinforced path as well
// as discover new paths.
#define EXPLORATORY_MESSAGE_DELAY    60      // sec bw sends

// This parameter specifies how much time to wait before sending
// a positive reinforcement message in response to an exploratory
// data message.
#define POS_REINFORCEMENT_SEND_DELAY  600
#define POS_REINFORCEMENT_JITTER      200

#ifdef USE_BROADCAST_MAC

//
// ----------------------> Please read this ! <----------------------
//
// The following parameters are for a broadcast mac. When using
// non-broadcast macs (e.g. WINSNG 2.0 nodes), please look below.

// Change the following parameters to set how long to wait between
// receiving an interest message from a neighbor and forwarding it.
#define INTEREST_FORWARD_DELAY   2000        // (msec) bw receive and forward
#define INTEREST_FORWARD_JITTER  1500        // (msec) jitter

// Change the following parameters to set how long to wait between
// receiving an exploratory data message from a neighbor and forwarding it.
#define DATA_FORWARD_DELAY        300        // (msec) bw receive and forward
#define DATA_FORWARD_JITTER       200        // (msec) jitter

#else

// The following parameters are for using diffusion with
// a non-broadcast mac.

// Change the following parameters to set how long to wait between
// receiving an interest message from a neighbor and forwarding it.
#define INTEREST_FORWARD_DELAY   1000        // (msec) bw receive and forward
#define INTEREST_FORWARD_JITTER   750        // (msec) jitter

// Change the following parameters to set how long to wait between
// receiving an exploratory data message from a neighbor and forwarding it.
#define DATA_FORWARD_DELAY        150        // (msec) bw receive and forward
#define DATA_FORWARD_JITTER       100        // (msec) jitter

#endif // USE_BROADCAST_MAC

// The following timeouts are used for determining when gradients,
// subscriptions, filters and neighbors should expire. These are
// mostly a function of other parameters and should not be changed.
#define SUBSCRIPTION_TIMEOUT    (INTEREST_DELAY/1000 * 3)         // sec
#define GRADIENT_TIMEOUT        (INTEREST_DELAY/1000 * 5)         // sec
#define FILTER_TIMEOUT          (FILTER_KEEPALIVE_DELAY/1000 * 2) // sec
#define NEIGHBORS_TIMEOUT       (INTEREST_DELAY/1000 * 4)         // sec

// The following parameters are not dependent of radio properties
// and just specify how often diffusion/other modules check for events.
#define GRADIENT_DELAY          10000        // (msec) bw checks
#define REINFORCEMENT_DELAY     10000        // (msec) bw checks
#define SUBSCRIPTION_DELAY      10000        // (msec) bw checks
#define NEIGHBORS_DELAY         10000        // (msec) bw checks
#define FILTER_DELAY            60000        // (msec) bw checks
#define FILTER_KEEPALIVE_DELAY  20000        // (msec) bw sends

// Number of packets to keep in the hash table
#define HASH_TABLE_MAX_SIZE           1000
#define HASH_TABLE_REMOVE_AT_ONCE       20
#define HASH_TABLE_DATA_MAX_SIZE       100
#define HASH_TABLE_DATA_REMOVE_AT_ONCE  10
