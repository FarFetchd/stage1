#include <stdio.h>
#include <math.h>

// Assumes that engine performance stays at sea level values.

float start_mass_tons; // metric
float thrust_asl_kN, thrust_asl_N;
float thrust_vac_kN, thrust_vac_N;
float mass_burn_per_sec_solid; // solid fuel units
float mass_burn_per_sec_lfo; // just the liquid fuel component (units, not mass)
                             // of LF+O
float burnout_time; // seconds until first engine(s) run out of fuel
float drag_surface; // the 2D profile of the front of the craft, m^2
float drag_coeff; // unitless, crazy guestimate

float cur_mass; // kg
float burn_rate; // kg / s
float cur_v = 0.0f;
float cur_x = 70.0f; // KSC elevation!
const float STEP = 0.025f;

// Takes meters, returns Kelvin
float temp_at(float altitude)
{
    if(altitude <= 11000.0f) // 285 to 205
        return 285.0f - (altitude/11000.0f) * 80.0f;
    else if(altitude <= 39000.0f) // 205 to 270
        return 205.0f + ((altitude - 11000.0f)/28000.0f) * 65.0f;
    else
        return 270.0f - (altitude - 39000.0f) * (50.0f / 16000.0f);
}

// Takes meters, returns Pa
float pressure_at(float altitude)
{
    return 101325.0f * exp(-altitude / 5600.0f);
}

// Takes meters, returns atm
float atm_at(float altitude)
{
    return exp(-altitude / 5600.0f);
}

// Takes meters, returns Newtons
float thrust_at(float altitude)
{
    return thrust_asl_N + (thrust_vac_N - thrust_asl_N)
                          * (1.0f - atm_at(altitude));
}

// Returns velocity gained this period.
float step_v(float stepsize)
{
    float avg_mass = (cur_mass + (cur_mass - burn_rate*stepsize)) / 2.0f;
    float ret = (thrust_at(cur_x) / avg_mass) * stepsize;
    cur_mass -= burn_rate * stepsize;
    return ret;
}

// Returns altitude gained this period.
// Pass in velocity at period's beginning and end.
float step_x(float stepsize, float v_0, float v_1)
{
    float avg_v = (v_0 + v_1) / 2.0f;
    return avg_v * stepsize;
}

float cur_drag()
{
    return 0.5f * (pressure_at(cur_x) / (287.053 * temp_at(cur_x)))
                * cur_v * cur_v * drag_surface * drag_coeff;
}

int main(int argc, char** argv)
{
    if(argc != 9)
    {
        fprintf(stderr,
"Usage: %s start_mass(metric tons) thrust_ASL(kN) thrust_vac(kN) \
solid_fuel_per_sec(fuel units) LFO_per_sec(liquid fuel units) \
time_to_1st_burnout(seconds)  drag_frontal_area(m^2) drag_coeff(unitless)\n",
                argv[0]);
        return 1;
    }
    sscanf(argv[1], "%f", &start_mass_tons);
    sscanf(argv[2], "%f", &thrust_asl_kN);
    sscanf(argv[3], "%f", &thrust_vac_kN);
    sscanf(argv[4], "%f", &mass_burn_per_sec_solid);
    sscanf(argv[5], "%f", &mass_burn_per_sec_lfo);
    sscanf(argv[6], "%f", &burnout_time);
    sscanf(argv[7], "%f", &drag_surface);
    sscanf(argv[8], "%f", &drag_coeff);

    cur_mass = start_mass_tons * 1000.0f;
    // 1440 liquid units in LFO == 16000kg,
    // 11.11111kg per liquid unit in LFO
    // 7.5kg per solid fuel unit
    burn_rate = mass_burn_per_sec_solid * 7.5f + mass_burn_per_sec_lfo*11.1111f;
    thrust_asl_N = thrust_asl_kN * 1000.0f;
    thrust_vac_N = thrust_vac_kN * 1000.0f;

    for(float t = 0.0f; t < burnout_time; t+=STEP)
    {
        float v_gain = step_v(STEP);
        cur_x += step_x(STEP, cur_v, cur_v+v_gain);
        cur_v += v_gain;
        cur_v -= 10.0f * STEP; // gravity
        cur_v -= (cur_drag() / cur_mass) * STEP;
    }
    printf("altitude %f meters, vert speed %f m/s \n", cur_x, cur_v);
}

