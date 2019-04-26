//By Monica Moniot
#ifndef MACHINES__H_INCLUDE
#define MACHINES__H_INCLUDE

#include "types.hh"
#include "world.hh"
#include "render.hh"

struct ResoQ {
	ResoId id;
	double n;
};

void init_func_tables();

bool can_build_machine(const GameState* game, TileCoord coord, MachineId id);
void build_machine(GameState* game, TileCoord coord, MachineId id);
void remove_machine(GameState* game, TileCoord coord, MachineId id);

void process_all_machines(GameState* game, double dt);

#endif

#ifdef MACHINES_IMPLEMENTATION
#undef MACHINES_IMPLEMENTATION

#define DECL_CAN_BUILD(name) bool MACRO_CAT(can_build_, name)(const GameState* game, TileCoord coord)
typedef DECL_CAN_BUILD(func);
DECL_CAN_BUILD(stub) {return 0;}

#define DECL_BUILD(name) void MACRO_CAT(build_, name)(GameState* game, TileCoord coord)
typedef DECL_BUILD(func);
DECL_BUILD(stub) {return;}

#define DECL_DRAW(name) void MACRO_CAT(draw_, name)(const GameState* game, const Machine* machine, TileCoord coord, Output* output)
typedef DECL_DRAW(func);
DECL_DRAW(stub) {return;}

#define DECL_DRAW_GHOST(name) void MACRO_CAT(draw_ghost_, name)(const GameState* game, TileCoord coord, Output* output)
typedef DECL_DRAW_GHOST(func);
DECL_DRAW_GHOST(stub) {return;}


//////////////////////////////////////////////////
//function tables
//
static can_build_func* can_build_func_table[MACHINES_TOTAL];
static build_func* build_func_table[MACHINES_TOTAL];
static draw_func* draw_func_table[MACHINES_TOTAL];
static draw_ghost_func* draw_ghost_func_table[MACHINES_TOTAL];

bool can_build_machine(const GameState* game, TileCoord coord, MachineId id) {
	ASSERTL(can_build_func_table[id], "request for unregistered machine");
	return can_build_func_table[id](game, coord);
}
void build_machine(GameState* game, TileCoord coord, MachineId id) {
	ASSERTL(can_build_func_table[id], "request for unregistered machine");
	return build_func_table[id](game, coord);
}


constexpr float machine_tick_time = .1;
void process_all_machines(GameState* game) {
	//finish crafting from last tick
	for_each_in(machine_relptr, game->built_machines, game->built_machines_size) {
		Machine* machine = mam_get_ptr(Machine, &game->stack, *machine_relptr);
		float new_rate = 1;
		if(machine->rate > 0) {
			float effective_time = (machine_tick_time - machine->preprocessed_time)*machine->rate;
			for_each_lt(out_portno, machine->out_ports_size) {
				auto out_port = machine->out_ports[out_portno];
				if(out_port.machine_data) {
					auto out_machine = mam_get_ptr(Machine, &game->stack, out_port.machine_data);
					//TODO: allow machines to modify their own recipes
					out_machine->in_store[out_port.this_portno] += effective_time*machine->max_out[out_portno];
				} else {
					new_rate = 0;
				}
			}
		} else {
			//TODO: prevent wastage
			for_each_lt(out_portno, machine->out_ports_size) {
				auto out_port = machine->out_ports[out_portno];
				if(!out_port.machine_data) {
					new_rate = 0;
					break;
				}
			}
		}
		//prepare for next tick
		machine->preprocessed_time = 0;
		machine->rate = new_rate;
	}
	//At this point it could be made safe to allow machines to modify their recipes

	//set new rates by finding the maximum valid rate each machine can have
	for_each_in(machine_relptr, game->built_machines, game->built_machines_size) {
		Machine* machine = mam_get_ptr(Machine, &game->stack, *machine_relptr);
		for_each_lt(in_portno, machine->in_ports_size) {
			//find maximum rate allowed by in stores
			machine->rate = min(machine->rate, machine->in_store[in_portno]/(machine_tick_time*machine->max_in[in_portno]));
			//find maximum rate allowed by the output storage
			auto in_port = machine->in_ports[in_portno];
			if(in_port.machine_data) {
				auto in_machine = mam_get_ptr(Machine, &game->stack, in_port.machine_data);
				float sum = machine_tick_time*in_machine->max_out[in_port.this_portno];
				in_machine->rate = min(in_machine->rate, max(0.0f, machine->max_in_store[in_portno] - machine->in_store[in_portno])/sum);
			}
		}
	}
	//commit rates
	for_each_in(machine_relptr, game->built_machines, game->built_machines_size) {
		Machine* machine = mam_get_ptr(Machine, &game->stack, *machine_relptr);
		float effective_time = machine_tick_time*machine->rate;
		for_each_lt(in_portno, machine->in_ports_size) {
			machine->in_store[in_portno] -= effective_time*machine->max_in[in_portno];
		}
	}
}

MachinePort get_machine_port_at(GameState* game, vec2 pos, bool* ret_port_is_in) {
	//TODO: search adjacent squares, maybe the port extends across a machine boundary
	MachinePort ret = {};
	float radius = .2;
	Tile machine_tile = world_get_machine(game, to_coordv(pos));
	if(machine_tile.id != MACHINE_EMPTY) {
		auto machine = mam_get_ptr(Machine, &game->stack, machine_tile.data);
		for_each_lt(portno, machine->in_ports_size) {
			vec2 port_pos = machine->in_port_poses[portno];
			if(vec2_length(port_pos - pos) < radius) {
				ret.machine_data = machine_tile.data;
				ret.this_portno = portno;
				if(ret_port_is_in) *ret_port_is_in = 1;
				return ret;
			}
		}
		for_each_lt(portno, machine->out_ports_size) {
			vec2 port_pos = machine->out_port_poses[portno];
			if(vec2_length(port_pos - pos) < radius) {
				ret.machine_data = machine_tile.data;
				ret.this_portno = portno;
				if(ret_port_is_in) *ret_port_is_in = 0;
				return ret;
			}
		}
	}
	return ret;
}

bool connect_machine_ports(GameState* game, MachinePort port0, bool port0_is_in, MachinePort port1, bool port1_is_in) {
	if(port0_is_in^port1_is_in) {
		if(port0_is_in) {
			//make sure port0 is an out port
			swap(&port0, &port1);
		}
		auto machine0 = mam_get_ptr(Machine, &game->stack, port0.machine_data);
		auto machine1 = mam_get_ptr(Machine, &game->stack, port1.machine_data);
		if(machine0->out_reso_ids[port0.this_portno] == machine1->in_reso_ids[port1.this_portno]) {
			machine0->out_ports[port0.this_portno] = port1;
			machine1->in_ports[port1.this_portno] = port0;
			return 1;
		}
	}
	return 0;
}

void draw_machine(GameState* game, Machine* machine, TileCoord coord, MachineId id, Output* output) {
	ASSERTL(draw_func_table[id], "request for unregistered machine");
	return draw_func_table[id](game, machine, coord, output);
}
void draw_ghost_machine(GameState* game, TileCoord coord, MachineId id, Output* output) {
	ASSERTL(draw_ghost_func_table[id], "request for unregistered machine");
	return draw_ghost_func_table[id](game, coord, output);
}

/*
machine {
	rate,
	preprocessed_time,
	in_ports_size;
	out_ports_size;
	max_in[#in_ports],
	in_store[#in_ports],
	in_ports[#in_ports],
	max_out[#out_ports],
	out_ports[#out_ports],
}
partial_process(machine, time_since_last_tick) {
	for each out_port, out_machines in machine.out_ports do
		for each this_port, out_machine in out_machines do
			out_machine.in_store[this_port] <= out_machine.in_store[this_port] + (time_since_last_tick - machine.preprocessed_time)*machine.rate*machine.max_out[out_port]
		end
	end
	machine.preprocessed_time <= time_since_last_tick
}
process() {
	//we assume all ports are connected
	//we also assume all ports have non-zero production
	define machine_tick_time
	//finish crafting from last tick
	for each machine in machines do
		for each out_port, out_machines in machine.out_ports do
			for each this_port, out_machine in out_machines do
				out_machine.in_store[this_port] <= out_machine.in_store[this_port] + (machine_tick_time - machine.preprocessed_time)*machine.rate*machine.max_out[out_port]
			end
		end
		machine.preprocessed_time <= 0
		machine.rate <= 1
	end
	//set new rates by finding the maximum valid rate each machine can have
	for each machine in machines do
		for each in_port, in_machines in machine.in_ports do
			//find maximum rate allowed by in stores
			if machine.rate > machine.in_store[in_port]/(machine_tick_time*machine.max_in[in_port]) do
				machine.rate <= machine.in_store[in_port]/(machine_tick_time*machine.max_in[in_port])
			end
			//find maximum rate allowed for it's inputs by the in_store
			define sum <= 0
			for each this_port, in_machine in in_machines do
				sum <= sum + machine_tick_time*in_machine.max_out[this_port]
			end
			for each this_port, in_machine in in_machines do
				if sum > 0 and in_machine.rate > (get_max_in_store(machine) - machine.in_store[in_port])/sum do
					in_machine.rate <= (get_max_in_store(machine) - machine.in_store[in_port])/sum
				end
			end
		end
	end
	//commit rates
	for each machine in machines do
		for each in_port in machine.in_ports do
			//NOTE: make sure this never results in negative store
			machine.in_store[in_port] <= machine.in_store[in_port] - machine_tick_time*machine.rate*machine.max_in[in_port]
		end
	end
}

machine {
	max_in[#portsin],
	max_out[#portsout],
	resos_in[#portsin],
	resos_out[#portsout];
	in_ports[#portsin],
	out_ports[#portsout],
}
port {
	max,
	cur,
	machines,
}

set_craft_rate do
	for each in_port in machine.in_ports do
		machine.resos_in[in_port] <= rate*machine.max_in[in_port]
	end
	for each out_port in machine.out_ports do
		machine.resos_out[out_port] <= rate*machine.max_out[out_port]
	end
end

find_equilibrium() {
	for each machine in machines do
		set_craft_rate(machine, 1)
	end
	define stable <= 0
	while not stable do
		stable = 1
		for each machine in machines do
			for each in_port, in_machines in machine.in_ports do
				define max_sum <= 0
				define sum <= 0
				for each this_port, in_machine in in_machines do
					max_sum <= max_sum + in_machine.max_out[this_port]
					sum <= sum + in_machine.resos_out[this_port]
				end
				if machine.resos_in[in_port] != sum do
					stable = 0
					if machine.resos_in[in_port] < sum do
						for each in_machine, this_port in in_machines do
							set_craft_rate(in_machine, machine.resos_in[in_port]/max_sum)
						end
					else
						set_craft_rate(machine, sum/machine.max_in[in_port])
					end
				end
			end
		end
	end
}
*/
//////////////////////////////////////////////////
// standards
//
inline bool can_build_costs(const GameState* game, const ResoQ* costs, uint32 costs_size) {
	auto reso_vec = game->reso_vec;
	for_each_in(it, costs, costs_size) {
		if(reso_vec[it->id] < it->n) {
			return 0;
		}
	}
	return 1;
}
inline void build_costs(GameState* game, const ResoQ* costs, uint32 costs_size) {
	auto reso_vec = game->reso_vec;
	for_each_in(it, costs, costs_size) {
		reso_vec[it->id] -= it->n;
	}
}

static void queue_machine(GameState* game, Tile machine_tile) {
	//TODO: variable memory for this array
	ASSERTL(game->built_machines_size <= BUILT_MACHINES_CAPACITY, "debug out of space for machines!");
	game->built_machines[game->built_machines_size] = machine_tile.data;
	game->built_machines_size += 1;
}

static void init_machine(Machine* machine, MachineId machine_id, float* max_in, int32* in_reso_ids, float* max_in_store, vec2* in_port_poses, uint32 in_ports_size, float* max_out, int32* out_reso_ids, vec2* out_port_poses, uint32 out_ports_size) {
	memzero(machine, sizeof(Machine));
	machine->id = machine_id;
	if(in_ports_size > 0) {
		machine->in_ports_size = in_ports_size;
		memcpy(&machine->max_in, max_in, sizeof(float)*in_ports_size);
		memcpy(&machine->in_reso_ids, in_reso_ids, sizeof(int32)*in_ports_size);
		memcpy(&machine->max_in_store, max_in_store, sizeof(float)*in_ports_size);
		memcpy(&machine->in_port_poses, in_port_poses, sizeof(vec2)*in_ports_size);
	}
	if(out_ports_size > 0) {
		machine->out_ports_size = out_ports_size;
		memcpy(&machine->max_out, max_out, sizeof(float)*out_ports_size);
		memcpy(&machine->out_reso_ids, out_reso_ids, sizeof(int32)*out_ports_size);
		memcpy(&machine->out_port_poses, out_port_poses, sizeof(vec2)*out_ports_size);
	}
}

//////////////////////////////////////////////////
// RENDER
//

static void draw_machine_box(Output* output, TileCoord pos, vec4 color) {
	draw_box(&output->world_graphics, gb_vec2(pos.x - .5, pos.y - .5), gb_vec2(pos.x + .5, pos.y + .5), COLOR_GRAY);
	draw_box(&output->world_graphics, gb_vec2(pos.x - .3, pos.y - .3), gb_vec2(pos.x + .3, pos.y + .3), color);
}

static void draw_ghost_machine_box(Output* output, TileCoord pos, vec4 color, bool is_valid) {
	draw_box(&output->world_graphics, gb_vec2(pos.x - .5, pos.y - .5), gb_vec2(pos.x + .5, pos.y + .5), is_valid ? COLOR_GRAY_TRANS : COLOR_RED_TRANS);
	draw_box(&output->world_graphics, gb_vec2(pos.x - .3, pos.y - .3), gb_vec2(pos.x + .3, pos.y + .3), color);
}

enum Direction {
	RIGHT,
	LEFT,
	UP,
	DOWN,
	CENTER,
};
static vec2 get_port_pos(vec2 origin, Direction d) {
	float distance = .5;
	if(d == RIGHT) {
		origin = gb_vec2(origin.x + distance, origin.y);
	} else if(d == LEFT) {
		origin = gb_vec2(origin.x - distance, origin.y);
	} else if(d == UP) {
		origin = gb_vec2(origin.x, origin.y + distance);
	} else if(d == DOWN) {
		origin = gb_vec2(origin.x, origin.y - distance);
	} else if(d == CENTER) {
	} else {
		ASSERT(0);
	}
	return origin;
}
static void draw_machine_ports(Output* output, const Machine* machine) {
	float radius0 = .25;
	float radius1 = .2;
	for_each_in(port_pos, machine->in_port_poses, machine->in_ports_size) {
		draw_circle(&output->world_graphics, *port_pos, radius0, COLOR_BLACK);
		draw_circle(&output->world_graphics, *port_pos, radius1, COLOR_GREEN);
	}
	for_each_in(port_pos, machine->out_port_poses, machine->out_ports_size) {
		draw_circle(&output->world_graphics, *port_pos, radius0, COLOR_BLACK);
		draw_circle(&output->world_graphics, *port_pos, radius1, COLOR_YELLOW);
	}
}
static void draw_port_connections(const GameState* game, Output* output, const Machine* machine) {
	//this must only be called on machine output ports
	for_each_lt(in_portno, machine->in_ports_size) {
		MachinePort in_port = machine->in_ports[in_portno];
		if(in_port.machine_data) {
			vec2 line0 = machine->in_port_poses[in_portno];
			Machine* in_machine = mam_get_ptr(Machine, &game->stack, in_port.machine_data);
			vec2 line1 = in_machine->out_port_poses[in_port.this_portno];
			draw_line(&output->world_graphics, line0, line1, .01, COLOR_WHITE);
		}
	}
}


//////////////////////////////////////////////////
// MACHINE_MINER
//

DECL_CAN_BUILD(miner) {
	auto game_stack = &game->stack;
	auto world = &game->world;
	Tile cur_tile = world_get_tile(game, coord);

	ResoQ costs[1] = {{IRON, 50.0},};
	return can_build_costs(game, costs, 1) && (cur_tile.id == TILE_IRON_ORE || cur_tile.id == TILE_COPPER_ORE);
}
DECL_DRAW(miner) {
	draw_machine_box(output, coord, COLOR_BLACK);
	draw_machine_ports(output, machine);
	draw_port_connections(game, output, machine);
}
DECL_DRAW_GHOST(miner) {
	draw_ghost_machine_box(output, coord, COLOR_BLACK, can_build_miner(game, coord));
}
DECL_BUILD(miner) {
	auto game_stack = &game->stack;
	auto world = &game->world;
	Tile cur_tile = world_get_tile(game, coord);
	ResoQ costs[1] = {{IRON, 50.0},};
	build_costs(game, costs, 1);

	Tile machine_tile = {MACHINE_MINER, mam_stack_pushi(game_stack, sizeof(Machine))};
	auto miner = mam_get_ptr(Machine, game_stack, machine_tile.data);
	{
		float max_out[1] = {1.0};
		int32 out_resos[1] = {IRON_ORE};
		vec2 out_port_poses[1] = {get_port_pos(to_vec(coord), UP)};
		init_machine(miner, MACHINE_MINER, 0, 0, 0, 0, 0, max_out, out_resos, out_port_poses, 1);
	}
	// if(cur_tile.id == TILE_IRON_ORE) {
	// 	miner->tile = TILE_IRON_ORE;//record which tile it was built on
	// } else if(cur_tile.id == TILE_COPPER_ORE) {
	// 	miner->tile = TILE_COPPER_ORE;
	// }
	queue_machine(game, machine_tile);
	world_set_machine(game, coord, machine_tile);
}

/////////////////////////////////////////////////
// MACHINE_SMELTER
//

DECL_CAN_BUILD(smelter) {
	auto game_stack = &game->stack;
	auto world = &game->world;

	ResoQ costs[1] = {{IRON, 50.0},};
	return can_build_costs(game, costs, 1);
}
DECL_BUILD(smelter) {
	auto world = &game->world;

	ResoQ costs[1] = {{IRON, 50.0},};
	build_costs(game, costs, 1);

	Tile machine_tile = {MACHINE_SMELTER, mam_stack_pushi(&game->stack, sizeof(Machine))};
	auto smelter = mam_get_ptr(Machine, &game->stack, machine_tile.data);
	{
		float max_in[1] = {1.0};
		int32 in_resos[1] = {IRON_ORE};
		float max_in_store[1] = {2.0};
		vec2 in_port_poses[1] = {get_port_pos(to_vec(coord), DOWN)};
		float max_out[1] = {.9};
		int32 out_resos[1] = {IRON};
		vec2 out_port_poses[1] = {get_port_pos(to_vec(coord), UP)};
		init_machine(smelter, MACHINE_SMELTER, max_in, in_resos, max_in_store, in_port_poses, 1, max_out, out_resos, out_port_poses, 1);
	}

	queue_machine(game, machine_tile);
	world_set_machine(game, coord, machine_tile);
}

DECL_DRAW(smelter) {
	draw_machine_box(output, coord, COLOR_ORANGE);
	draw_machine_ports(output, machine);
	draw_port_connections(game, output, machine);
}
DECL_DRAW_GHOST(smelter) {
	draw_ghost_machine_box(output, coord, COLOR_ORANGE, can_build_smelter(game, coord));
}

/////////////////////////////////////////////////
// MACHINE_FACTORY
//

DECL_CAN_BUILD(factory) {
	auto world = &game->world;

	ResoQ costs[2] = {{IRON, 50.0}, {COPPER, 50.0}};
	return can_build_costs(game, costs, 2);
}
DECL_BUILD(factory) {
	auto world = &game->world;
	ResoQ costs[2] = {{IRON, 50.0}, {COPPER, 50.0}};
	build_costs(game, costs, 2);


	Tile machine_tile = {MACHINE_FACTORY, mam_stack_pushi(&game->stack, sizeof(Machine))};
	auto factory = mam_get_ptr(Machine, &game->stack, machine_tile.data);
	{
		float max_in[2] = {1.0, 2.0};
		int32 in_resos[2] = {IRON, COPPER};
		float max_in_store[2] = {2.0, 4.0};
		//NOTE: we need to actually choose proper port positions
		vec2 in_port_poses[2] = {get_port_pos(to_vec(coord), DOWN), get_port_pos(to_vec(coord), UP)};
		float max_out[1] = {3.0};
		int32 out_resos[1] = {CIRCUITRY};
		vec2 out_port_poses[1] = {get_port_pos(to_vec(coord), LEFT)};
		init_machine(factory, MACHINE_SMELTER, max_in, in_resos, max_in_store, in_port_poses, 2, max_out, out_resos, out_port_poses, 1);
	}
	queue_machine(game, machine_tile);

	for_each_lt(y, 3) {
		for_each_lt(x, 3) {
			TileCoord c = {x + coord.x, y + coord.y};
			world_set_machine(game, c, machine_tile);
		}
	}
}


DECL_DRAW_GHOST(factory) {
	for_each_lt(y, 3) {
		for_each_lt(x, 3) {
			TileCoord c = {x + coord.x, y + coord.y};
			draw_ghost_machine_box(output, c, COLOR_GREEN, 1);
		}
	}
}
DECL_DRAW(factory) {
	// for_each_lt(y, 3) {
	// 	for_each_lt(x, 3) {
			// TileCoord c = {x + coord.x, y + coord.y};
	draw_machine_box(output, coord, COLOR_GREEN);
	// 	}
	// }
	draw_machine_ports(output, machine);
	draw_port_connections(game, output, machine);
}
/////////////////////////////////////////////////
// REGISTER
//
#define REGISTER_MACHINE(name, id) {\
	can_build_func_table[id] = MACRO_CAT(can_build_, name);\
	build_func_table[id] = MACRO_CAT(build_, name);\
	draw_func_table[id] = MACRO_CAT(draw_, name);\
	draw_ghost_func_table[id] = MACRO_CAT(draw_ghost_, name);\
}
void init_func_tables() {
	REGISTER_MACHINE(stub, MACHINE_EMPTY);
	REGISTER_MACHINE(miner, MACHINE_MINER);
	REGISTER_MACHINE(smelter, MACHINE_SMELTER);
	REGISTER_MACHINE(factory, MACHINE_FACTORY);
}

#endif
