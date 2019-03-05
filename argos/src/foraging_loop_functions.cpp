#include "foraging_loop_functions.h"
#include <argos3/core/simulator/simulator.h>
#include <argos3/core/utility/configuration/argos_configuration.h>
#include <argos3/plugins/robots/foot-bot/simulator/footbot_entity.h>
#include <argos3/core/wrappers/lua/lua_controller.h>

const double nest_site_threshold = -1;
const int number_of_foraging_sites = 3;
const double foraging_site_radius = 0.25;
    
const double arena_width = 3.9;
const double arena_height = 3.9;

const double lower_x = nest_site_threshold + foraging_site_radius;
const double upper_x = (arena_height / 2) - foraging_site_radius;

const double lower_y = (-arena_width / 2) + foraging_site_radius;
const double upper_y = (arena_width / 2) - foraging_site_radius;

void CForagingLoopFunctions::Init(TConfigurationNode& t_node)
{
    int random_seed = 0;

    // Read random seed from the ARGoS configuration file
    try
    {
        TConfigurationNode& configuration_tree = CSimulator::GetInstance().GetConfigurationRoot();
        TConfigurationNode& framework_node = GetNode(configuration_tree, "framework");
        TConfigurationNode& experiment_node = GetNode(framework_node, "experiment");

        GetNodeAttribute(experiment_node, "random_seed", random_seed);
    }
    catch(CARGoSException& ex)
    {
        THROW_ARGOSEXCEPTION_NESTED("Error parsing random seed!", ex);
    }

    // Create a new category based on the random seed
    CRandom::CreateCategory("loop_functions_category", random_seed);

    // Create a new random number generator in this category
    random_number_generator = CRandom::CreateRNG("loop_functions_category");

    // Zero the food item counters
    for(int i = 0; i < number_of_foraging_sites; i++)
        food_item_counters[i] = 0;

    // Calculate possible foraging site locations, based on arena dimensions
    foraging_arena_side_x = CRange<Real>(lower_x, upper_x);
    foraging_arena_side_y = CRange<Real>(lower_y, upper_y);

    // Randomly place the foraging sites
    PlaceForagingSites();

    // Read parameters from the ARGoS configuration file
    try
    {
        TConfigurationNode& foraging_node = GetNode(t_node, "foraging"); 

        // Get the logging mode from the XML
        GetNodeAttribute(foraging_node, "include_csv_header", include_csv_header);

        // Get the logging mode from the XML
        GetNodeAttribute(foraging_node, "log_every_tick", log_every_tick);
      
        // Get the output file name from the XML
        GetNodeAttribute(foraging_node, "output_filename", output_filename);
      
        // Open the file, erasing its contents
        output_stream.open(output_filename.c_str(), std::ios_base::trunc | std::ios_base::out);

        // Write the CSV header
        if(include_csv_header)
            output_stream << header << std::endl;
    }
    catch(CARGoSException& ex)
    {
        THROW_ARGOSEXCEPTION_NESTED("Error parsing loop functions!", ex);
    }
}

void CForagingLoopFunctions::Reset()
{
    // Zero the food item counters
    for(int i = 0; i < number_of_foraging_sites; i++)
        food_item_counters[i] = 0;

    // Close the file
    output_stream.close();

    // Open the file, erasing its contents
    output_stream.open(output_filename.c_str(), std::ios_base::trunc | std::ios_base::out);

    // Write the CSV header
    if(include_csv_header)
        output_stream << header << std::endl;

    // Randomly place the foraging sites
    foraging_sites.clear();
    PlaceForagingSites();
}

void CForagingLoopFunctions::Destroy()
{
    // Close the file
    output_stream.close();
}

CColor CForagingLoopFunctions::GetFloorColor(const CVector2& position_on_plane)
{
    // Nest site is coloured grey
    if(position_on_plane.GetX() < nest_site_threshold)
        return CColor::GRAY50;

    // Foraging sites are coloured black
    for(CVector2& foraging_site : foraging_sites)
    {
        if((position_on_plane - foraging_site).Length() < foraging_site_radius)
            return CColor::BLACK;
    }

    // The rest of the floor is coloured white
    return CColor::WHITE;
}

void CForagingLoopFunctions::PreStep()
{
    // Only log data when a food item has been returned to the nest   
    bool log_data = false;

    // Check whether a robot is on a food item
    CSpace::TMapPerType& foot_bots = GetSpace().GetEntitiesByType("foot-bot");

    for(auto& map_element : foot_bots)
    {
        // Get handle to foot-bot entity and controller
        CFootBotEntity& foot_bot = *any_cast<CFootBotEntity*>(map_element.second);
        CLuaController& lua_controller = dynamic_cast<CLuaController&>(foot_bot.GetControllableEntity().GetController());

        // Get the position of the foot-bot on the ground as a CVector2
        CVector2 position;
        position.Set(foot_bot.GetEmbodiedEntity().GetOriginAnchor().Position.GetX(),
                     foot_bot.GetEmbodiedEntity().GetOriginAnchor().Position.GetY());

        lua_State* lua_state = lua_controller.GetLuaState();

        // Get 'robot' table
        lua_getglobal(lua_state, "robot");

        bool has_food = false;
        int food_value;

        // Check whether the robot has a food item
        if(!lua_isnil(lua_state, -1))
        {
            // Get 'has_food' variable
            lua_getfield(lua_state, -1, "has_food");

            // If 'has_food' variable found and its value is a boolean
            if((!lua_isnil(lua_state, -1)) && (lua_isboolean(lua_state, -1)))
                has_food = lua_toboolean(lua_state, -1); // Save the value to a local variable

            // Clean up Lua stack
            lua_pop(lua_state, 1);

            // Get 'food_value' variable
            lua_getfield(lua_state, -1, "food_value");

            // If 'food_value' variable found and its value is a number
            if((!lua_isnil(lua_state, -1)) && (lua_isnumber(lua_state, -1)))
                food_value = lua_tonumber(lua_state, -1); // Save the value to a local variable

            // Clean up Lua stack
            lua_pop(lua_state, 1);
        }

        // If the robot has a food item
        if(has_food)
        {
            // If the robot is in the nest
            if(position.GetX() < nest_site_threshold)
            {
                // Drop the food item
                lua_pushboolean(lua_state, false);
                lua_setfield(lua_state, -2, "has_food");

                // Clear the food item value
                lua_pushnumber(lua_state, 0);
                lua_setfield(lua_state, -2, "food_value");

                // Update food item counters
                food_item_counters[(food_value / 5) - 1]++;
                log_data = true;
            }
        }
        // If the robot doesn't have a food item
        else
        {
            // If the robot is out of the nest
            if(position.GetX() > nest_site_threshold)
            {
                for(int i = 0; i < foraging_sites.size(); i++)
                {
                    // Check whether the robot is over a foraging site
                    if((position - foraging_sites[i]).Length() < foraging_site_radius)
                    {
                        // The robot is now carrying a food item
                        lua_pushboolean(lua_state, true);
                        lua_setfield(lua_state, -2, "has_food");

                        // Food item value is based on foraging site index
                        lua_pushnumber(lua_state, (i + 1) * 5);
                        lua_setfield(lua_state, -2, "food_value");

                        break;
                    }
                }
            }
        }
    }

    // Log data to a file
    if(log_data || log_every_tick)
    {
        output_stream << GetSpace().GetSimulationClock() << ","
                      << food_item_counters[0] << ","
                      << food_item_counters[1] << ","
                      << food_item_counters[2] << ","
                      << food_item_counters[0] + food_item_counters[1] + food_item_counters[2] << ","
                      << food_item_counters[0] * 5 << ","
                      << food_item_counters[1] * 10 << ","
                      << food_item_counters[2] * 15 << ","
                      << (food_item_counters[0] * 5) + (food_item_counters[1] * 10) + (food_item_counters[2] * 15) << std::endl;

        output_stream.flush();
    }
}

void CForagingLoopFunctions::PlaceForagingSites()
{
    // Randomly distribute the three foraging sites
    for(int i = 0; i < number_of_foraging_sites; ++i)
    {
        bool problem;
        CVector2 new_foraging_site;

        do
        {
            // Try to randomly place a new foraging site            
            problem = false;

            new_foraging_site = CVector2(random_number_generator->Uniform(foraging_arena_side_x),
                                         random_number_generator->Uniform(foraging_arena_side_y));

            // If at least one foraging site has already been placed
            if(foraging_sites.size() > 0)
            {
                // Iterate over existing foraging sites
                for(CVector2& foraging_site : foraging_sites)
                {
                    double distance = (foraging_site - new_foraging_site).Length();

                    // If the new foraging site is too close to any existing foraging site, then randomly generate a new one
                    if(distance < foraging_site_radius * 4)
                        problem = true;
                }
            }

        } while(problem);

        // Add new foraging site to vector
        foraging_sites.push_back(new_foraging_site);
    }
}

REGISTER_LOOP_FUNCTIONS(CForagingLoopFunctions, "foraging_loop_functions")
