#include "foraging_qt_user_functions.h"
#include <argos3/core/wrappers/lua/lua_controller.h>

using namespace argos;

CForagingQTUserFunctions::CForagingQTUserFunctions()
{
    RegisterUserFunction<CForagingQTUserFunctions,CFootBotEntity>(&CForagingQTUserFunctions::Draw);
}

void CForagingQTUserFunctions::Draw(CFootBotEntity& entity)
{
    CLuaController& lua_controller = dynamic_cast<CLuaController&>(entity.GetControllableEntity().GetController());
    lua_State* lua_state = lua_controller.GetLuaState();

    // This is the message that will be shown above the robot
    std::string message;

    // This is the list of rays that will be drawn coming from the robot
    std::vector<CVector2> ray_coordinates;
    std::vector<CColor> ray_colours;

    bool has_food = false;
    int food_value = 0;

    // Get 'robot' table
    lua_getglobal(lua_state, "robot");

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

        // If the robot has food
        if(has_food)
        {
            CColor food_item_colour;

            // Food item colour is determined by its value
            switch(food_value)
            {
                case 5:
                    food_item_colour = CColor::ORANGE;
                    break;
                case 10:
                    food_item_colour = CColor::CYAN;
                    break;
                case 15:
                    food_item_colour = CColor::MAGENTA;
                    break;
            }

            // Draw a food item of the appropriate colour above the robot
            DrawCylinder(CVector3(0.0f, 0.0f, 0.3f),
                         CQuaternion(),
                         0.1f,
                         0.05f,
                         food_item_colour);
        }

        // Get 'message' variable
        lua_getfield(lua_controller.GetLuaState(), -1, "message");

        // If 'message' variable found and its value is a string
        if((!lua_isnil(lua_controller.GetLuaState(), -1)) && (lua_isstring(lua_controller.GetLuaState(), -1)))
            message = lua_tostring(lua_controller.GetLuaState(), -1); // Save the value to a local variable

        // Clean up Lua stack
        lua_pop(lua_controller.GetLuaState(), 1);

        // Get the 'vectors' table
        lua_getfield(lua_controller.GetLuaState(), -1, "vectors");

        // If 'vectors' variable found and its value is a table
        if((!lua_isnil(lua_controller.GetLuaState(), -1)) && (lua_istable(lua_controller.GetLuaState(), -1)))
        {
            // Go through the table elements
            lua_pushnil(lua_controller.GetLuaState());

            while(lua_next(lua_controller.GetLuaState(), -2))
            {
                // Is this element a table?
                if(lua_istable(lua_controller.GetLuaState(), -1))
                {
                    CVector2 vector_coordinates;
                    CColor vector_colour;

                    // Extract (x, y) coordinates and colour
                    lua_getfield(lua_controller.GetLuaState(), -1, "x");

                    if(!lua_isnil(lua_controller.GetLuaState(), -1) && lua_type(lua_controller.GetLuaState(), -1) == LUA_TNUMBER)
                        vector_coordinates.SetX(lua_tonumber(lua_controller.GetLuaState(), -1));

                    lua_pop(lua_controller.GetLuaState(), 1);
                    lua_getfield(lua_controller.GetLuaState(), -1, "y");

                    if(!lua_isnil(lua_controller.GetLuaState(), -1) && lua_type(lua_controller.GetLuaState(), -1) == LUA_TNUMBER)
                        vector_coordinates.SetY(lua_tonumber(lua_controller.GetLuaState(), -1));

                    lua_pop(lua_controller.GetLuaState(), 1);
                    lua_getfield(lua_controller.GetLuaState(), -1, "color");

                    if(!lua_isnil(lua_controller.GetLuaState(), -1) && lua_type(lua_controller.GetLuaState(), -1) == LUA_TSTRING)
                        vector_colour.Set(lua_tostring(lua_controller.GetLuaState(), -1));

                    lua_pop(lua_controller.GetLuaState(), 1);

                    // Add item to lists
                    ray_coordinates.push_back(vector_coordinates);
                    ray_colours.push_back(vector_colour);
                }

                // Done with element
                lua_pop(lua_controller.GetLuaState(), 1);
            }
        }

        // Clean up Lua stack
        lua_pop(lua_controller.GetLuaState(), 2);
    }

    // Disable face culling to be sure the vectors and text are visible from anywhere
    glDisable(GL_CULL_FACE);

    // Draw rays
    for(int i = 0; i < ray_coordinates.size(); i++)
    {
        DrawRay(CRay3(CVector3(0.0, 0.0, 0.3),
                      CVector3(ray_coordinates[i].GetX(), ray_coordinates[i].GetY(), 0.3)),
                ray_colours[i],
                5);
    }

    // Disable lighting, so it doesn't interfere with the chosen text colour
    glDisable(GL_LIGHTING);

    // Set the text colour
    CColor text_colour(CColor::BLACK);
    glColor3ub(text_colour.GetRed(), text_colour.GetGreen(), text_colour.GetBlue());

    // The position of the text is expressed with respect to the reference point of the foot-bot
    // For a foot-bot, the reference point is the centre of its base.
    // See also the description in:
    // $ argos3 -q foot-bot
    DrawText(CVector3(0.0, 0.0, 0.4), message.c_str());

    // Restore lighting
    glEnable(GL_LIGHTING);

    // Restore face culling
    glEnable(GL_CULL_FACE);
}

REGISTER_QTOPENGL_USER_FUNCTIONS(CForagingQTUserFunctions, "foraging_qt_user_functions")
