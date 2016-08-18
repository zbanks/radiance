#include <ctime>
#include <cmath>
#include <functional>
#include <thread>
#include <mutex>
#include <utility>
#include "util/common.h"
#include <tuple>
#include "util/config.h"
#include "util/err.h"
#include "util/math.h"

#include "time/timebase.h"

#define N_MAX_TIME_SOURCES 8

struct time_master time_master;
double phase = 0.0;

namespace {
int64_t time_rdtscp(uint32_t auxv = 0u)
{
    return __rdtscp(&auxv);
}
struct tsc_initializer {
    std::atomic<double>  m_tsc_rate{ 0.};
    std::atomic<double>  m_tsc_rate_inv{ 0.};
    int64_t m_tsc_offset = 0;
    std::mutex m_mtx;
    tsc_initializer(int64_t initialize_for_ns = 1000000 )
    {
        auto ts = timespec{ 0, initialize_for_ns };
        auto ret = 0;
        do {
            m_tsc_offset = time_rdtscp();
            ret = nanosleep(&ts, nullptr);
            m_tsc_rate = (time_rdtscp() - m_tsc_offset) * (1e9/initialize_for_ns);
        }while(ret < 0 && errno == EINTR);
        if(m_tsc_rate)
            m_tsc_rate_inv = initialize_for_ns * 1 / m_tsc_rate;
    }
    double nanosleep_estimate_tsc(int64_t sleep_for_ns)
    {
        auto ts = timespec{ 0, sleep_for_ns};
        auto tsc_start = time_rdtscp();
        if(nanosleep(&ts, nullptr)) {
            return -errno;
        }else{
            return (time_rdtscp() - tsc_start) * (1e9/sleep_for_ns);
        }
    }
    double nanosleep_update_tsc(int64_t sleep_for_ns)
    {
        auto res = nanosleep_estimate_tsc(sleep_for_ns);
        if(res >= 0){
            auto _lock = std::unique_lock<std::mutex>(m_mtx, std::try_to_lock);
            auto old = m_tsc_rate.load();
            if(_lock.owns_lock()) {
                auto rate = 0.5 * (old + res);
                auto rinv = 1. / rate;
                m_tsc_rate.store(rate);
                m_tsc_rate_inv.store(rinv);
            }
            return std::abs((old - res) / (2 * (old + res)));
        }else{
            return res;
        }
    }
    std::tuple<double,double,double,double>
    self_test(int64_t duration_per_rep = 1000000, size_t reps = 16, size_t limit = 0)
    {
        auto samples = std::vector<double>{};
        samples.reserve(reps);
        auto rep = size_t{0};
        while(samples.size() < reps && ( !limit || (rep++) < limit)) {
            auto sample = nanosleep_estimate_tsc(duration_per_rep);
            if(sample >= 0)
                samples.push_back(sample);
        }
        auto _sum = 0.;
        auto c = 0.;
        for(auto sample : samples) {
            auto y = sample - c;
            auto t = _sum + y;
                 c = (t - _sum) - y;
            _sum = t;
        }
        auto _avg = _sum / samples.size();
        _sum = 0.;

        auto _max = samples.front();
        auto _min = samples.front();
        auto square = [](auto x){return x*x;};
        for(auto sample : samples) {
            auto y = square(sample - _avg) - c;
            auto t = _sum + y;
                 c = (t - _sum) - y;
            _sum = t;
            _min = std::min(_min,sample);
            _max = std::max(_max,sample);
        }
        auto _var = _sum / (samples.size() - 1);
        auto _std = std::sqrt(_var);
        return std::make_tuple(_min,_avg,_max,_std);
    }
    static tsc_initializer &instance()
    {
        static tsc_initializer l_instance(100000000);
        return l_instance;
    }
    int64_t ref_cycles() const
    {
        return time_rdtscp() - m_tsc_offset;
    } 
    double frequency() const
    {
        return m_tsc_rate;
    }
    double frequency_inverse() const
    {
        return m_tsc_rate_inv;
    }
    double  time() const
    {
        return ref_cycles() * frequency_inverse();
    }
};
}
double time_cpu_frequency(void)
{
    return tsc_initializer::instance().frequency();
}
double time_cpu_frequency_inverse(void)
{
    return tsc_initializer::instance().frequency_inverse();
}
double time_cpu_time(void)
{
    return tsc_initializer::instance().time();
}
double time_cpu_nanosleep_estimate(int64_t ns)
{
    return tsc_initializer::instance().nanosleep_estimate_tsc(ns);
}
double time_cpu_nanosleep_update(int64_t ns)
{
    return tsc_initializer::instance().nanosleep_update_tsc(ns);
}
int64_t time_cpu_ref_cycles(void)
{
    return tsc_initializer::instance().ref_cycles();
}
// Maximum size of `time_master.beat_index`. 16 to track 4/4 beats+bars
//static uint8_t master_beat_denominator = 16;

// Utility functions
static double error_wrap(double x_est, double x_obs) {
    // Error of `x_est` from `x_obs` in the space [-1, 1]
    // D(0.1, 0) == -D(0.9, 0) == 0.1
    // D(0.5, 0) == +0.5; D(0.51, 0) == -0.49
    auto d = x_est - x_obs + 100;
    return std::fmod(d + 0.5, 1.0) - 0.5;
}

static void error_wrap_test()
{
    for(auto x = -0.95; x <= 1.0; x += 0.05) {
        for (auto d = -0.99; d <= 1.0; d += 0.01) {
            auto e = x + d;
            while(e >= 1.0)
                e -= 1.0;
            while(e < 0)
                e += 1.0;
            auto r = error_wrap(e, x);
            if (std::abs(r - d) > 1e6)
                FAIL("x=%lf, d=%lf, e=%lf, r=%lf", x, d, e, r);
        }
    }
}

// Time master
int time_init()
{
    memset(&time_master, 0, sizeof time_master);
    time_master.bpm = 140;
    INFO("Current timestamp counter rate is %E\n",time_cpu_frequency());
    INFO("Current timestamp offset is %E\n s", time_cpu_time());
    auto _min = 0., _mean = 0., _max = 0., _std = 0.;
    std::tie(_min,_mean,_max,_std) = tsc_initializer::instance().self_test(1000000, 8, 128);
    INFO("TSC SELF TEST: min = %E, mean = %E, max = %E, std = %E (GHz) ( relative std = %E %%\n",
        _min * 1e-9,_mean * 1e-9,_max * 1e-9,_std * 1e-9, _std / _mean * 1e2
        );
    auto percentage_error = [](auto x, auto y){return (x - y) / ( 2 * (x + y));};
    auto average_in = [](auto &x, auto y){return (x + y) / 2;};
    if(std::abs(percentage_error(_mean,time_cpu_frequency())) > 1e-2) {
        WARN("TSC Estimate fluctuated by > 1%%\n");
    }else{
        auto &instance = tsc_initializer::instance();
        average_in(instance.m_tsc_rate, _mean);
        instance.m_tsc_rate_inv = 1./ instance.m_tsc_rate;
    }
    error_wrap_test();
    return 0;
}

void time_term() { return; }

void time_update(enum time_source source, enum time_source_event event, double event_arg)
{
    struct timespec tv = {0, 0};
    if (clock_gettime(CLOCK_MONOTONIC_RAW, &tv) != 0) {
        PERROR("clock_gettime failed");
        return;
    }

    // Convert to milliseconds
    auto time_ms = tv.tv_sec * 1000 + tv.tv_nsec / (1000 * 1000);

    auto delta_time_ms = time_ms - time_master.wall_ms;
    if (time_master.wall_ms == 0)
        delta_time_ms = 0;
    (void) delta_time_ms;

    switch (event) {
    case TIME_SOURCE_EVENT_BAR:
        break; //TODO
    case TIME_SOURCE_EVENT_BPM:
        time_master.bpm = event_arg;
        break;
    case TIME_SOURCE_EVENT_BEAT: {
        ;
        auto ms_until_event = event_arg;
        auto master_beat_per_ms = time_master.bpm * MINUTES_PER_MILLISECOND;
        char status = '!';
        (void) status;
        if (event_arg == 0) {
            time_master.beat_frac = 0;
            time_master.beat_index++;
        } else if (event_arg < 0) { // How long ago was the last beat
            time_master.beat_frac = -ms_until_event * master_beat_per_ms;
            status = '<';
        } else {
            time_master.beat_frac = MAX(time_master.beat_frac, 1.0 - ms_until_event * master_beat_per_ms);
            status = '>';
        }
#ifdef WHAT_IS_GOING_ON
        INFO("Beat: %c %0.1f %u %0.3f", status, time_master.bpm, time_master.beat_index, time_master.beat_frac);
#endif
        break;
    }
    case TIME_SOURCE_EVENT_NONE:
    default:
        break;
    }

#ifdef MATH_IS_HARD
    // Convert bpm to beat/ms
    double master_beat_per_ms = time_master.bpm / MINUTES_PER_MILLISECOND;
    phase += delta_time_ms * master_beat_per_ms;

    auto phase_est_at_event = phase + master_beat_per_ms * ms_until_event;
    auto error = error_wrap(phase_est_at_event, 0.0);
    time_master.bpm += error * 0.5;

    static auto n_errors = 0;
    static auto total_error = 0;
    n_errors++;
    total_error += std::abs(error);

    // Update time_master
    auto phase_idx = (long) phase;
    if (phase > 1.0) {
        phase -= phase_idx;
        INFO("Predicted %ld beat(s) at bpm=%lf error=%lf", phase_idx, time_master.bpm, total_error / n_errors);
        n_errors = 0;
        total_error = 0;
    } else {
        phase_idx = 0;
    }

    time_master.beat_frac = phase;
    time_master.beat_index += phase_idx;
#endif

    time_master.wall_ms = time_ms;
}
