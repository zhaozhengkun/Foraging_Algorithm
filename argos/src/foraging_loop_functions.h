#ifndef FORAGING_LOOP_FUNCTIONS_H
#define FORAGING_LOOP_FUNCTIONS_H

#include <argos3/core/simulator/loop_functions.h>
#include <argos3/core/utility/math/range.h>
#include <argos3/core/utility/math/rng.h>

using namespace argos;

class CForagingLoopFunctions : public CLoopFunctions
{
public:

    CForagingLoopFunctions() {}
    virtual ~CForagingLoopFunctions() {}

    virtual void Init(TConfigurationNode& t_tree);
    virtual void Reset();
    virtual void Destroy();
    virtual CColor GetFloorColor(const CVector2& c_position_on_plane);
    virtual void PreStep();

private:

    void PlaceForagingSites();

    CRandom::CRNG* random_number_generator;

    int food_item_counters[3];
    std::vector<CVector2> foraging_sites;
    CRange<Real> foraging_arena_side_x, foraging_arena_side_y;

    bool include_csv_header;
    bool log_every_tick;

    std::string output_filename;
    std::ofstream output_stream;
    std::string header = "tick,count_5,count_10,count_15,count_total,value_5,value_10,value_15,value_total";
};

#endif
