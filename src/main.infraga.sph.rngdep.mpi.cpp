#include <stdlib.h>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <cstring>
#include <sstream>
#include <math.h>
#include <vector>
#include <mpi.h>

#include "atmo/atmo_state.h"
#include "atmo/atmo_io.sph.rngdep.h"

#include "geoac/geoac.params.h"
#include "geoac/geoac.eqset.h"
#include "geoac/geoac.interface.h"
#include "geoac/geoac.eigenray.mpi.h"

#include "util/fileIO.h"
#include "util/globe.h"
#include "util/interpolation.h"
#include "util/multitasking.h"
#include "util/rk4solver.h"

using namespace std;

void version(){
    cout << '\n' << '\t' << "infraga v 1.0.3" << '\n';
    cout << '\t' << "Copyright (c) 2014, Triad National Security, LLC.  All rights reserved." << '\n';
    cout << '\t' << "This software was produced under U.S. Government contract 89233218CNA000001 "<< '\n';
    cout << '\t' << "for Los Alamos National Laboratory (LANL), which is operated by Triad" << '\n';
    cout << '\t' << "National Security, LLC, for the U.S. Department of Energy/National Nuclear" << '\n';
    cout << '\t' << "Security Administration." << '\n' << '\n';
    
    cout << '\t' << "All rights in the program are reserved by Triad National Security, LLC, and " << '\n';
    cout << '\t' << "the U.S. Department of Energy/National Nuclear Security Administration. The" << '\n';
    cout << '\t' << "Government is granted for itself and others acting on its behalf a nonexclusive," << '\n';
    cout << '\t' << "paid-up, irrevocable worldwide license in this material to reproduce, prepare " << '\n';
    cout << '\t' << "derivative works, distribute copies to the public, perform publicly and display" << '\n';
    cout << '\t' << "publicly, and to permit others to do so." << '\n' << '\n';
    
    cout << '\t' << "License: Open Source MIT <http://opensource.org/licenses/MIT>" << '\n';
    cout << '\t' << "This is free software: you are free to change and redistribute it." << '\n';
    cout << '\t' << "The software is provided 'AS IS', without warranty of any kind, expressed or implied." << '\n';
    cout << '\t' << "See manual for full licence information." << '\n' << '\n';
}

void usage(){
    cout << '\n';
    cout << '\t' << "###########################################################" << '\n';
    cout << '\t' << "####              infraga-accel-sph-rngdep             ####" << '\n';
    cout << '\t' << "####      Accelerated Ray Tracing Using Spherical      ####" << '\n';
    cout << '\t' << "####  Coordinates and an Inhomogeneous, Moving Medium  ####" << '\n';
    cout << '\t' << "###########################################################" << '\n' << '\n';
    
    cout << '\n';
    cout << "Usage: mpirun -np [number] infraga-accel-sph-rngdep [option] profile_id nodes-lat.dat nodes-lon.dat [parameters]" << '\n';
    cout << '\t' << '\t' << "Enter only 1 option." << '\n';
    cout << '\t' << '\t' << "Each profile_id##.met is expected to contain columns describing {z[km]  T[K]  u (zonal wind) [m/s]  v (meridional wind) [m/s]  density[g/cm^3]  p[mbar]} " << '\n';
    cout << '\t' << '\t' << "The files nodes-lat.dat and nodes-lon.dat are expected to contain columns describing the latitude and longitude locations of the .met files." << '\n';
    cout << '\t' << '\t' << '\t' << "These can be generated using the included Python script for a single .bin file." << '\n';
    cout << '\t' << '\t' << '\t' << "Profiles are ordered such that profile_id[N].met describes the atmosphere at lat = lat_i, lon = lon_j, N = i + n_lat * j" << '\n';
    cout << '\t' << '\t' << '\t' << "Profile format can be modified, see manual document for details." << '\n';
    cout << '\t' << '\t' << "Parameter calls are expected using the format: parameter_name=value." << '\n' << '\n';
    
    cout << "Options and parameters are:" << '\n';
    cout << '\t' << "-prop (generate ray paths for propagations at multiple azimuth and inclination angles)" << '\n';
    cout << '\t' << '\t' << "Parameter"  << '\t' << "Units" << '\t' << '\t' << "Default Value" << '\n';
    cout << '\t' << '\t' << "---------------------------------------------" << '\n';
    cout << '\t' << '\t' << "incl_min"          << '\t' << "degrees"            << '\t' << '\t' << "0.5" << '\n';
    cout << '\t' << '\t' << "incl_max"          << '\t' << "degrees"            << '\t' << '\t' << "45.0" << '\n';
    cout << '\t' << '\t' << "incl_step"         << '\t' << "degrees"            << '\t' << '\t' << "0.5"  << '\n';
    cout << '\t' << '\t' << "inclination"       << '\t' << "see manual"         << '\t' << "0.5" << '\n';
    cout << '\t' << '\t' << "az_min"            << '\t' << '\t' << "degrees"    << '\t' << '\t' << "-90.0" << '\n';
    cout << '\t' << '\t' << "az_max"            << '\t' << '\t' << "degrees"    << '\t' << '\t' << "-90.0" << '\n';
    cout << '\t' << '\t' << "az_step"           << '\t' << '\t' << "degrees"    << '\t' << '\t' << "1.0"  << '\n';
    cout << '\t' << '\t' << "azimuth"           << '\t' << '\t' << "see manual" << '\t' << "-90.0" << '\n';
    cout << '\t' << '\t' << "bounces"           << '\t' << '\t' << "integer"    << '\t' << '\t' << "10" << '\n';
    cout << '\t' << '\t' << "src_lat"           << '\t' << '\t' << "degrees"    << '\t' << '\t' << "midpoint of nodes-lat file" << '\n';
    cout << '\t' << '\t' << "src_lon"           << '\t' << '\t' << "degrees"    << '\t' << '\t' << "midpoint of nodes-lon file" << '\n';
    cout << '\t' << '\t' << "src_alt"           << '\t' << '\t' << "km"         << '\t' << '\t' << "0.0" << '\n';

    cout << '\t' << '\t' << "turn_ht_min"       << '\t' << "km"                 << '\t' << '\t' << "0.2" << '\n';
    cout << '\t' << '\t' << "write_rays"        << '\t' << "true/false"         << '\t' << "false" << '\n';
    cout << '\t' << '\t' << "write_topo"        << '\t' << "true/false"         << '\t' << "false" << '\n' << '\n';
    
    cout << '\t' << "-eig_search (Search for all eigenrays connecting a source at (src_lat, src_lon, src_alt) to a receiver " << '\n';
    cout << '\t' << '\t' << '\t' << "at (rcvr_lat, rcvr_lon, z_grnd) which have inclinations and ground reflections within specified limits)" << '\n';
    cout << '\t' << '\t' << "Parameter"  << '\t' << "Units" << '\t' << '\t' << "Default Value" << '\n';
    cout << '\t' << '\t' << "---------------------------------------------" << '\n';
    cout << '\t' << '\t' << "incl_min"         << '\t' << "degrees"            << '\t' << '\t' << "1.0" << '\n';
    cout << '\t' << '\t' << "incl_max"         << '\t' << "degrees"            << '\t' << '\t' << "45.0" << '\n';
    cout << '\t' << '\t' << "bnc_min"           << '\t' << '\t' << "integer"    << '\t' << '\t' << "0" << '\n';
    cout << '\t' << '\t' << "bnc_max"           << '\t' << '\t' << "integer"    << '\t' << '\t' << "0" << '\n';
    cout << '\t' << '\t' << "bounces"           << '\t' << '\t' << "see manual" << '\t' << "0" << '\n';
    
    cout << '\t' << '\t' << "src_lat"           << '\t' << '\t' << "degrees"    << '\t' << '\t' << "midpoint of nodes-lat file" << '\n';
    cout << '\t' << '\t' << "src_lon"           << '\t' << '\t' << "degrees"    << '\t' << '\t' << "midpoint of nodes-lon file" << '\n';
    cout << '\t' << '\t' << "src_alt"           << '\t' << '\t' << "km"         << '\t' << '\t' << "0.0" << '\n';
    cout << '\t' << '\t' << "srcs_list"          << '\t' << "see manual"        << '\t' << "none" << '\n';
    cout << '\t' << '\t' << "rcvr_lat"          << '\t' << "degrees"            << '\t' << '\t' << "midpoint of nodes-lat file" << '\n';
    cout << '\t' << '\t' << "rcvr_lon"          << '\t' << "degrees"            << '\t' << '\t' << "midpoint of nodes-lon file + 5.0" << '\n';
    cout << '\t' << '\t' << "rcvs_list"          << '\t' << "see manual"        << '\t' << "none" << '\n';

    cout << '\t' << '\t' << "seq_srcs"          << '\t' << "true/false"         << '\t' << "false" << '\n';
    cout << '\t' << '\t' << "seq_srcs_dr"       << '\t' << "km"                 << '\t' << '\t' << "5.0" << '\n';

    cout << '\t' << '\t' << "verbose"           << '\t' << '\t' << "true/false" << '\t' << "false" << '\n';
    cout << '\t' << '\t' << "verbose_rcvr"       << '\t' << "integer"           << '\t' << '\t' << "0" << '\n';
    cout << '\t' << '\t' << "iterations"        << '\t' << "integer"            << '\t' << '\t' << "25" << '\n';
    cout << '\t' << '\t' << "damping"           << '\t' << '\t' << "scalar"     << '\t' << '\t' << "1.0e-3" << '\n';
    cout << '\t' << '\t' << "tolerance"         << '\t' << "km"                 << '\t' << '\t' << "0.1" << '\n';
    cout << '\t' << '\t' << "az_dev_lim"        << '\t' << "degrees"            << '\t' << '\t' << "2.0" << '\n';
    cout << '\t' << '\t' << "incl_step_min"     << '\t' << "degrees"            << '\t' << '\t' << "0.001" << '\n';
    cout << '\t' << '\t' << "incl_step_min"     << '\t' << "degrees"            << '\t' << '\t' << "0.001" << '\n' << '\n';

    cout << '\t' << "Additional Parameters"  << '\t' << "Units/Options" << '\t' << "Default Value" << '\n';
    cout << '\t' << "---------------------------------------------" << '\n';
    cout << '\t' << "freq"              << '\t' << '\t' << '\t' << "Hz"         << '\t' << '\t' << "0.1" << '\n';
    cout << '\t' << "abs_coeff"         << '\t' << '\t' << "scalar"             << '\t' << '\t' << "0.3" << '\n';
    cout << '\t' << "z_grnd"            << '\t' << '\t' << '\t' << "km"         << '\t' << '\t' << "0.0" << '\n';
    cout << '\t' << "write_atmo"        << '\t' << '\t' << "true/false"         << '\t' << "false" << '\n';
    cout << '\t' << "prof_format"   	<< '\t' << '\t' << "see manual"         << '\t' << "zTuvdp" << '\n';
    cout << '\t' << "output_id"         << '\t' << '\t' << "see manual"         << '\t' << "from profile.met" << '\n';
    cout << '\t' << "calc_amp"          << '\t' << '\t' << "true/false"         << '\t' << "true" << '\n';

    cout << '\t' << "max_alt"           << '\t' << '\t' << '\t' << "km"         << '\t' << '\t' << "interpolation limits" << '\n';
    cout << '\t' << "max_rng"           << '\t' << '\t' << '\t' << "km"         << '\t' << '\t' << "1000.0" << '\n';
    cout << '\t' << "max_time"          << '\t' << '\t' << '\t' << "hr"         << '\t' << '\t' << "10.0" << '\n';
    cout << '\t' << "min_lat"           << '\t' << '\t' << '\t' << "degrees"    << '\t' << '\t' << "interpolation limits" << '\n';
    cout << '\t' << "max_lat"           << '\t' << '\t' << '\t' << "degrees"    << '\t' << '\t' << "interpolation limits" << '\n';
    cout << '\t' << "min_lon"           << '\t' << '\t' << '\t' << "degrees"    << '\t' << '\t' << "interpolation limits" << '\n';
    cout << '\t' << "max_lon"           << '\t' << '\t' << '\t' << "degrees"    << '\t' << '\t' << "interpolation limits" << '\n';
    cout << '\t' << "min_ds"            << '\t' << '\t' << '\t' << "km"         << '\t' << '\t' << "0.001" << '\n';
    cout << '\t' << "max_ds"            << '\t' << '\t' << '\t' << "km"         << '\t' << '\t' << "0.05" << '\n';
    cout << '\t' << "max_s"             << '\t' << '\t' << '\t' << "km"         << '\t' << '\t' << "1000.0" << '\n';
    cout << '\t' << "topo_file"         << '\t' << '\t' << "see manual"         << '\t' << "none" << '\n';
    cout << '\t' << "topo_use_BLw"      << '\t' << '\t' << "see manual"         << '\t' << "false" << '\n' << '\n';
    
    cout << "Output (see output files or manual for units):" << '\n';
    cout << '\t' << "atmo.dat -> z[km] : c [m/s]  : u (zonal winds) [m/s] : v (meridional winds) [m/s] : density[g/cm^3] : ceff [km/s]" << '\n';
    cout << '\t' << "{...}.raypaths.dat -> lat : lon : z : trans. coeff. : absorption : time " << '\n';
    cout << '\t' << "{...}.arrivals.dat -> incl : az : n_bnc : lat : lon : time : cel : z_max : arrival incl : back az : trans. coeff. : absorption" << '\n' << '\n';
    
    cout << "Examples:" << '\n';
    cout << '\t' << "mpirun -np 6 ./bin/infraga-accel-sph-rngdep -prop examples/profs/example examples/profs/example_lat.dat examples/profs/example_lon.dat src_lat=40.0 src_lon=-102.5 azimuth=-90.0 write_rays=true" << '\n';
    cout << '\t' << "mpirun -np 6 ./bin/infraga-accel-sph-rngdep -eig_search examples/profs/example examples/profs/example_lat.dat examples/profs/example_lon.dat src_lat=40.0 src_lon=-102.5 rcvr_lat=41.05 rcvr_lon=-108.25 bnc_max=1 verbose=true" << '\n' << '\n';
}

void run_prop(char* inputs[], int count){
    int mpi_error, world_size, world_rank;
    mpi_error = MPI_Init(&count, &inputs);
    mpi_error = MPI_Comm_size(MPI_COMM_WORLD, &world_size);
    mpi_error = MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
    MPI_Status mpi_status;
    
    int int_buffer[1], break_check_int, mpi_break_check[world_size], mpi_k[world_size], ray_buffer_size = 6, results_buffer_size = 12;
    double ray_buffer[ray_buffer_size], results_buffer[results_buffer_size];
    
    if (world_rank == 0){
        cout << '\n';
        cout << '\t' << "##############################################" << '\n';
        cout << '\t' << "####   Running infraga-accel-sph-rngdep   ####" << '\n';
        cout << '\t' << "####              Propagation             ####" << '\n';
        cout << '\t' << "##############################################" << '\n' << '\n';
    }
    
    double theta_min = 0.5, theta_max = 45.0, theta_step = 0.5;
    double phi_min = -90.0, phi_max = -90.0, phi_step = 1.0;
    int bounces = 2, file_check;
    double  lat_src, lon_src, z_src = 0.0;
    bool write_atmo = false, write_rays = false, write_topo=false, custom_output_id=false;
    double freq = 0.1, turn_ht_min = 0.2;
    char* prof_format = "zTuvdp";
    char* topo_file = "None";
    char* output_id;
    char input_check;

    topo::z0 = 0.0;
    atmo::tweak_abs = 1.0;
    
    geoac::calc_amp = true;
    geoac::is_topo = false;
    
    for(int i = 5; i < count; i++){
        if (strncmp(inputs[i], "prof_format=", 12) == 0){       prof_format = inputs[i] + 12;}
        else if (strncmp(inputs[i], "z_grnd=", 7) == 0){        topo::z0 = atof(inputs[i] + 7);}
        else if (strncmp(inputs[i], "topo_file=", 10) == 0){    topo_file = inputs[i] + 10; geoac::is_topo=true;}
    }
    
    if (geoac::is_topo){    file_check = set_region(inputs[2], inputs[3], inputs[4], topo_file, prof_format, false, world_rank);}
    else{                   file_check = set_region(inputs[2], inputs[3], inputs[4], prof_format, false, world_rank);}
    if(!file_check){
        MPI_Finalize();
        return;
    }

    lat_src = (geoac::lat_max + geoac::lat_min) / 2.0 * (180.0 / Pi);
    lon_src = (geoac::lon_max + geoac::lon_min) / 2.0 * (180.0 / Pi);
    
    for(int i = 5; i < count; i++){
        if ((strncmp(inputs[i], "incl_min=", 9) == 0) || (strncmp(inputs[i], "min_incl=", 9) == 0)){        theta_min = atof(inputs[i] + 9);}
        else if ((strncmp(inputs[i], "incl_max=", 9) == 0) || (strncmp(inputs[i], "max_incl=", 9) == 0)){   theta_max = atof(inputs[i] + 9);}
        else if ((strncmp(inputs[i], "az_min=", 7) == 0) || (strncmp(inputs[i], "min_az=", 7) == 0)){       phi_min = atof(inputs[i] + 7);}
        else if ((strncmp(inputs[i], "az_max=", 7) == 0) || (strncmp(inputs[i], "max_az=", 7) == 0)){       phi_max = atof(inputs[i] + 7);}
        
        else if (strncmp(inputs[i], "incl_step=", 10) == 0){                                                theta_step = atof(inputs[i] + 10);}
        else if (strncmp(inputs[i], "az_step=", 8) == 0){                                                   phi_step = atof(inputs[i] + 8);}
        
        else if (strncmp(inputs[i], "inclination=", 12) == 0){                                              theta_min = atof(inputs[i] + 12);
                                                                                                            theta_max = atof(inputs[i] + 12);
                                                                                                            theta_step = 1.0;}
        else if (strncmp(inputs[i], "azimuth=", 8) == 0){                                                   phi_min = atof(inputs[i] + 8);
                                                                                                            phi_max = atof(inputs[i] + 8);
                                                                                                            phi_step = 1.0;}
        
        else if (strncmp(inputs[i], "bounces=", 8) == 0){                                                   bounces = atoi(inputs[i] + 8);}
        
        else if ((strncmp(inputs[i], "src_alt=", 8) == 0) || (strncmp(inputs[i], "alt_src=", 8) == 0)){     z_src = atof(inputs[i] + 8);}
        else if ((strncmp(inputs[i], "src_lat=", 8) == 0) || (strncmp(inputs[i], "lat_src=", 8) == 0)){     lat_src = atof(inputs[i] + 8);}
        else if ((strncmp(inputs[i], "src_lon=", 8) == 0) || (strncmp(inputs[i], "lon_src=", 8) == 0)){     lon_src = atof(inputs[i] + 8);}
        
        else if (strncmp(inputs[i], "turn_ht_min=", 12) == 0){                                              turn_ht_min = atof(inputs[i] + 12);}
        else if (strncmp(inputs[i], "write_rays=", 11) == 0){                                               write_rays = string2bool(inputs[i] + 11);}
        
        else if (strncmp(inputs[i], "freq=", 5) == 0){                                                      freq = atof(inputs[i] + 5);}
        else if (strncmp(inputs[i], "abs_coeff=", 10) == 0){                                                atmo::tweak_abs = max(0.0, atof(inputs[i] + 10));}
        else if (strncmp(inputs[i], "calc_amp=", 9) == 0){                                                  geoac::calc_amp = string2bool(inputs[i] + 9);}

        else if ((strncmp(inputs[i], "max_alt=", 8) == 0) || (strncmp(inputs[i], "alt_max=", 8) == 0)){     geoac::alt_max = atof(inputs[i] + 8);               if (world_rank == 0){cout << '\t' << "User selected altitude maximum = " << geoac::alt_max << '\n';}}
        else if ((strncmp(inputs[i], "max_rng=", 8) == 0) || (strncmp(inputs[i], "rng_max=", 8) == 0)){     geoac::rng_max = atof(inputs[i] + 8);               if (world_rank == 0){cout << '\t' << "User selected range maximum = " << geoac::rng_max << '\n';}}
        else if ((strncmp(inputs[i], "max_time=", 9) == 0) || (strncmp(inputs[i], "time_max=", 9) == 0)){   geoac::time_max = atof(inputs[i] + 9);              if (world_rank == 0){cout << '\t' << "User selected propagation time maximum = " << geoac::time_max << '\n';}}

        else if ((strncmp(inputs[i], "min_lat=", 8) == 0) || (strncmp(inputs[i], "lat_min=", 8) == 0)){     geoac::lat_min = atof(inputs[i] + 8) * Pi / 180.0;  if (world_rank == 0){cout << '\t' << "User selected latitude minimum = " << geoac::lat_min * (180.0 / Pi) << '\n';}}
        else if ((strncmp(inputs[i], "max_lat=", 8) == 0) || (strncmp(inputs[i], "lat_max=", 8) == 0)){     geoac::lat_max = atof(inputs[i] + 8) * Pi / 180.0;  if (world_rank == 0){cout << '\t' << "User selected latitude maximum = " << geoac::lat_max * (180.0 / Pi) << '\n';}}
        else if ((strncmp(inputs[i], "min_lon=", 8) == 0) || (strncmp(inputs[i], "lon_min=", 8) == 0)){     geoac::lon_min = atof(inputs[i] + 8) * Pi / 180.0;  if (world_rank == 0){cout << '\t' << "User selected longitude minimum = " << geoac::lon_min * (180.0 / Pi) << '\n';}}
        else if ((strncmp(inputs[i], "max_lon=", 8) == 0) || (strncmp(inputs[i], "lon_max=", 8) == 0)){     geoac::lon_max = atof(inputs[i] + 8) * Pi / 180.0;  if (world_rank == 0){cout << '\t' << "User selected longitude maximum = " << geoac::lon_max * (180.0 / Pi) << '\n';}}
        
        else if ((strncmp(inputs[i], "min_ds=", 7) == 0) || (strncmp(inputs[i], "ds_min=", 7) == 0)){       geoac::ds_min = atof(inputs[i] + 7);                if (world_rank == 0){cout << '\t' << "User selected ds minimum = " << geoac::ds_min << '\n';}}
        else if ((strncmp(inputs[i], "max_ds=", 7) == 0) || (strncmp(inputs[i], "ds_max=", 7) == 0)){       geoac::ds_max = atof(inputs[i] + 7);                if (world_rank == 0){cout << '\t' << "User selected ds maximum = " << geoac::ds_max << '\n';}}
        else if ((strncmp(inputs[i], "max_s=", 6) == 0) || (strncmp(inputs[i], "s_max=", 6) == 0)){         geoac::s_max = atof(inputs[i] + 6);                 if(world_rank == 0){cout << '\t' << "User selected s maximum (path length between reflections) = " << geoac::s_max << '\n';}}

        else if (strncmp(inputs[i], "write_atmo=", 11) == 0){                                               write_atmo = string2bool(inputs[i] + 11);}
        else if (strncmp(inputs[i], "prof_format=", 12) == 0){ 		                                        prof_format = inputs[i] + 12;}
        else if (strncmp(inputs[i], "output_id=", 10) == 0){                                                custom_output_id = true; 
                                                                                                            output_id = inputs[i] + 10;}
        else if (strncmp(inputs[i], "z_grnd=", 7) == 0){                                                    if(!geoac::is_topo){
                                                                                                                topo::z0 = atof(inputs[i] + 7);
                                                                                                                topo::z_max = topo::z0;
                                                                                                                topo::z_bndlyr = topo::z0 + 2.0;
                                                                                                            } else {
                                                                                                                cout << '\t' << "Note: cannot adjust ground elevation with topography." << '\n';
                                                                                                            }}
        else if (strncmp(inputs[i], "topo_file=",10) == 0){                                                 topo_file = inputs[i] + 10; geoac::is_topo=true;}
        else if (strncmp(inputs[i], "topo_use_BLw=", 13) == 0){                                             topo::use_BLw = string2bool(inputs[i] + 13);}
        else if (strncmp(inputs[i], "write_topo=", 11) == 0){                                               write_topo = string2bool(inputs[i] + 11);}
        else{
            if (world_size == 0){
                cout << "***WARNING*** Unrecognized parameter entry: " << inputs[i] << '\n';
            }
            mpi_error = MPI_Barrier(MPI_COMM_WORLD);
            MPI_Finalize();
            return;
        }
    }
    lat_src *= Pi / 180.0;
    lon_src *= Pi / 180.0;
    
    // Set the source height at topographical ground above sea level
    z_src = max(z_src, topo::z(lat_src, lon_src) - globe::r0);
    
    geoac::configure();

    if (world_rank == 0){
        cout << '\n' << "Parameter summary:" << '\n';
        cout << '\t' << "inclination: " << theta_min << ", " << theta_max << ", " << theta_step << '\n';
        cout << '\t' << "azimuth: " << phi_min << ", " << phi_max << ", " << phi_step << '\n';
        cout << '\t' << "bounces: " << bounces << '\n';
        cout << '\t' << "source location (lat, lon, alt): " << lat_src * (180.0 / Pi) << ", " << lon_src * (180.0 / Pi) << ", " << z_src << '\n';
        if(!geoac::is_topo){
            cout << '\t' << "ground elevation: " << topo::z0 << '\n';
        }
        cout << '\t' << "frequency: " << freq << '\n';
        cout << '\t' << "S&B atten coeff: " << atmo::tweak_abs << '\n';
    
        if (write_atmo){        cout << '\t' << "write_atmo: true" << '\n';}        else { cout << '\t' << "write_atmo: false" << '\n';}
        if (write_rays){        cout << '\t' << "write_rays: true" << '\n';}        else { cout << '\t' << "write_rays: false" << '\n';}
        if (write_topo){        cout << '\t' << "write_topo: true" << '\n';}        else { cout << '\t' << "write_topo: false" << '\n';}
        if (geoac::calc_amp){   cout << '\t' << "calc_amp: true" << '\n';}          else { cout << '\t' << "calc_amp: false" << '\n';}
        cout << '\t' << "threads: " << world_size << '\n' << '\n';
    }
    // Extract the file name from the input and use it to distinguish the resulting output
    char output_buffer [512];
    if(!custom_output_id){
        output_id = inputs[2];
    }
   
    // Define variables used for analysis
	double D, D_prev, travel_time_sum, attenuation, r_max, inclination, GC1, GC2, back_az;
	int k, length = int(geoac::s_max / geoac::ds_min);
	bool break_check;
    
	ofstream results, raypath, caustics, topo_out;

    if (world_rank == 0) {
        if(write_atmo){
            geoac::write_prof("atmo.dat", lat_src, lon_src, (90.0 - phi_min) * Pi / 180.0);
        }
        
        sprintf(output_buffer, "%s.arrivals.dat", output_id);
        results.open(output_buffer);

        results << "# infraga-accel-sph-rngdep -prop summary:" << '\n';
        results << "#" << '\t' << "profile prefix: " << inputs[2] << '\n';
        results << "#" << '\t' << "profile grid files: " << inputs[3] << ", " << inputs[4] << '\n';
        results << "#" << '\t' << "inclination: " << theta_min << ", " << theta_max << ", " << theta_step << '\n';
        results << "#" << '\t' << "azimuth: " << phi_min << ", " << phi_max << ", " << phi_step << '\n';
        results << "#" << '\t' << "bounces: " << bounces << '\n';
        results << "#" << '\t' << "source location (lat, lon, alt): " << lat_src * (180.0 / Pi) << ", " << lon_src * (180.0 / Pi) << ", " << z_src << '\n';
        if(!geoac::is_topo){
            results << "#" << '\t' << "ground elevation: " << topo::z0 << '\n';
        } else {
            results << "#" << '\t' << "topo file:" << topo_file << '\n';
        }
        results << "#" << '\t' << "frequency: " << freq << '\n';
        results << "#" << '\t' << "S&B atten coeff: " << atmo::tweak_abs << '\n';
        results << "#" << '\t' << "threads: " << world_size << '\n' << '\n';

        results << "# incl [deg]";
        results << '\t' << "az [deg]";
        results << '\t' << "n_b";
        results << '\t' << "lat_0 [deg]";
        results << '\t' << "lon_0 [deg]";
        results << '\t' << "time [s]";
        results << '\t' << "cel [km/s]";
        results << '\t' << "turning ht [km]";
        results << '\t' << "inclination [deg]";
        results << '\t' << "back azimuth [deg]";
        results << '\t' << "trans. coeff. [dB]";
        results << '\t' << "absorption [dB]";
        results << '\n';
    
        if(write_rays){
            sprintf(output_buffer, "%s.raypaths.dat", output_id);
            raypath.open(output_buffer);
        
            raypath << "# lat [deg]";
            raypath << '\t' << "lon [deg]";
            raypath << '\t' << "z [km]";
            raypath << '\t' << "trans. coeff. [dB]";
            raypath << '\t' << "absorption [dB]";
            raypath << '\t' << "time [s]";
            raypath << '\n';
        }
    }
    
	double** solution;
    geoac::build_solution(solution,length);
    mpi_error = MPI_Barrier(MPI_COMM_WORLD);
    
    for(double phi = phi_min;       phi <= phi_max;     phi += phi_step){
        geoac::phi = Pi / 2.0 - phi * (Pi / 180.0);

        double dzgdx, dzgdy, theta_grnd;
        if (z_src - (topo::z(lat_src, lon_src) - globe::r0) < 0.01){
            if (geoac::is_topo){
                dzgdx = 1.0 / (globe::r0 * cos(lat_src)) * topo::dz(lat_src, lon_src, 1);
                dzgdy = 1.0 / globe::r0 * topo::dz(lat_src, lon_src, 0);
                theta_grnd = atan(dzgdx * sin(phi * Pi / 180.0) + dzgdy * cos(phi * Pi / 180.0)) * (180.0 / Pi) + 0.1;
            } else {
                theta_grnd = 0.1;
            }
        } else {
            theta_grnd=-89.9;
        }

        for(double theta = max(theta_min, theta_grnd); theta <= max(theta_max, theta_grnd); theta += theta_step * world_size){
    		geoac::theta = (theta + world_rank * theta_step) * (Pi / 180.0);
        
            geoac::set_initial(solution, z_src, lat_src, lon_src);
            travel_time_sum = 0.0; attenuation = 0.0; r_max = 0.0;

            for(int j = 0; j < world_size; j++){
                if(theta + theta_step * j > theta_max){
                    mpi_break_check[j] = true;
                } else {
                    mpi_break_check[j] = false;
                }
            }
            mpi_error = MPI_Barrier(MPI_COMM_WORLD);
        
            if(world_rank == 0){
                cout << "Calculating ray paths: (" << theta << ", " << min(theta + (world_size - 1) * theta_step, theta_max);
                cout << ") degrees inclination range, " << phi << " degrees azimuth." << '\n';
            }

            if((fabs(theta - max(theta_min, theta_grnd)) < theta_step) && write_topo && world_rank == 0){
                sprintf(output_buffer, "%s.terrain.dat", output_id);
                topo_out.open(output_buffer);
            }

            for(int bnc_cnt = 0; bnc_cnt <= bounces; bnc_cnt++){
                if(!mpi_break_check[world_rank]){
                    k = geoac::prop_rk4(solution, break_check, length);
                } else {
                    break_check = true;
                }

                if(k < 2){
                    break_check = true;
                }

                mpi_error = MPI_Barrier(MPI_COMM_WORLD);            
                MPI_Allgather(&k, 1, MPI_INT, mpi_k, 1, MPI_INT, MPI_COMM_WORLD);

                if(write_rays){
                    for (int j = 0; j < world_size; j++){
                        if(!mpi_break_check[j]){
                            for(int m = 1; m < mpi_k[j] ; m++){
                                if(world_rank == j){
                                    geoac::travel_time(travel_time_sum, solution, m - 1, m);
                                    geoac::atten(attenuation, solution, m - 1, m, freq);
                                }
                            
                                if(m == 1 || m % 15 == 0){
                                    if (world_rank == j){
                                        ray_buffer[0] = solution[m][1] * (180.0 / Pi);
                                        ray_buffer[1] = solution[m][2] * (180.0 / Pi);
                                        ray_buffer[2] = solution[m][0] - globe::r0;
                                        if(geoac::calc_amp){
                                            ray_buffer[3] = 20.0 * log10(geoac::amp(solution, m));
                                        } else {
                                            ray_buffer[3] = 0.0;
                                        }
                                        ray_buffer[4] = -2.0 * attenuation;
                                        ray_buffer[5] = travel_time_sum;
                                    }
                                
                                    if(world_rank == j && j != 0){
                                        mpi_error = MPI_Send(ray_buffer, ray_buffer_size, MPI_DOUBLE, 0, 101, MPI_COMM_WORLD);
                                    } else if (world_rank == 0 && j != 0){
                                        mpi_error = MPI_Recv(ray_buffer, ray_buffer_size, MPI_DOUBLE, j, 101, MPI_COMM_WORLD, &mpi_status);
                                    }
                                
                                    if(world_rank == 0){
                                        raypath << ray_buffer[0];
                                        for(int n = 1; n < ray_buffer_size; n++){
                                            if(n != 0 && n != 1){
                                                raypath << '\t' << ray_buffer[n];
                                            } else {
                                                raypath << '\t' << setprecision(8) << ray_buffer[n];
                                            }
                                        }
                                        raypath << '\n';
                                    }
                                }
                            }
                            mpi_error = MPI_Barrier(MPI_COMM_WORLD);
                            if(world_rank == 0){
                                raypath << '\n';
                            }
                        }
                    }
                    mpi_error = MPI_Barrier(MPI_COMM_WORLD);
                } else {
                    travel_time_sum += geoac::travel_time(solution, k);
                    attenuation += geoac::atten(solution, k, freq);
                }

                if((fabs(theta - max(theta_min, theta_grnd)) < theta_step) && write_topo && (world_rank == 0)){
                    for(int m = 1; m < k ; m+=10){                        
                        topo_out << setprecision(8) << solution[m][1] * (180.0 / Pi);
                        topo_out << '\t' << setprecision(8) << solution[m][2] * (180.0 / Pi);
                        topo_out << '\t' << topo::z(solution[m][1], solution[m][2]) - globe::r0 << '\n';
                    }
                }

                for(int m = 0; m < k ; m++){
                    r_max = max (r_max, solution[m][0] - globe::r0);
                }

                if(!geoac::is_topo && r_max < (topo::z0 + turn_ht_min)){
                    break_check = true;
                }

                if(k < 2 || (travel_time_sum / 3600.0) > geoac::time_max){
                    break_check = true;
                }

                break_check_int = int(break_check);
                MPI_Allgather(&break_check_int, 1, MPI_INT, mpi_break_check, 1, MPI_INT, MPI_COMM_WORLD);

                inclination = asin(atmo::c(solution[k][0], solution[k][1], solution[k][2]) / atmo::c(globe::r0 + z_src, lat_src, lon_src) * solution[k][3]) * 180.0 / Pi;
                back_az = 90.0 - atan2(-solution[k][4], -solution[k][5]) * 180.0 / Pi;
                if(back_az < -180.0){
                    back_az += 360.0;
                } else if(back_az >  180.0){
                    back_az -= 360.0;
                }
            
                for (int j = 0; j < world_size; j++){
                    if(!mpi_break_check[j]){
                        if(world_rank == j){
                            results_buffer[0] = theta + theta_step * j;
                            results_buffer[1] = phi;
                            results_buffer[2] = bnc_cnt;
                            results_buffer[3] = solution[k][1] * (180.0 / Pi);
                            results_buffer[4] = solution[k][2] * (180.0 / Pi);
                            results_buffer[5] = travel_time_sum;
                            results_buffer[6] = globe::gc_dist(solution[k][1], solution[k][2], lat_src, lon_src) / travel_time_sum;
                            results_buffer[7] = r_max;
                            results_buffer[8] = inclination;
                            results_buffer[9] = back_az;
                            if(geoac::calc_amp){
                                results_buffer[10] = 20.0 * log10(geoac::amp(solution, k));
                            } else {
                               results_buffer[10] = 0.0;
                            }
                            results_buffer[11] = -2.0 * attenuation;
                        }
                    
                        if(world_rank == j && j!=0){
                            mpi_error = MPI_Send(results_buffer, results_buffer_size, MPI_DOUBLE, 0, 101, MPI_COMM_WORLD);
                        } else if (world_rank == 0 && j!=0){
                            mpi_error = MPI_Recv(results_buffer, results_buffer_size, MPI_DOUBLE, j, 101, MPI_COMM_WORLD, &mpi_status);
                        }
                    
                        mpi_error = MPI_Barrier(MPI_COMM_WORLD);
                        if(world_rank==0){
                            results << results_buffer[0];
                            for(int n = 1; n < results_buffer_size; n++){
                                if (n != 3 && n != 4){
                                    results << '\t' << results_buffer[n];
                                } else {
                                    results << '\t' << setprecision(8) << results_buffer[n];
                                }
                            }   
                            results << '\n';
                        }
                    }
                }   
                mpi_error = MPI_Barrier(MPI_COMM_WORLD);
                if(!mpi_break_check[world_rank]){
                    geoac::set_refl(solution,k);
                }
            }
            mpi_error = MPI_Barrier(MPI_COMM_WORLD);
            if(world_rank == 0 && write_rays){
                raypath << '\n';
            }

            if((fabs(theta - max(theta_min, theta_grnd)) < theta_step) && write_topo && (world_rank == 0)){
                topo_out.close();
            }

        }
    }

    if(world_rank==0){
        if(write_rays){
            raypath.close();
        }
        results.close();
    }

    geoac::delete_solution(solution, length);
    clear_region();
    
    mpi_error = MPI_Finalize();
}

void run_eig_search(char* inputs[], int count){
    // Define and set the rank and size in the world communicator
    int world_rank, world_size, group_rank, group_size, task_id;
    
    MPI_Init(&count, &inputs);
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);
    MPI_Comm_group(MPI_COMM_WORLD, &world_group);
    
    if(world_rank == 0){
        cout << '\n';
        cout << '\t' << "##############################################" << '\n';
        cout << '\t' << "####   Running infraga-accel-sph-rngdep   ####" << '\n';
        cout << '\t' << "####            Eigenray Search           ####" << '\n';
        cout << '\t' << "##############################################" << '\n' << '\n';
    }
    
    int srcs_cnt = 1, rcvrs_cnt = 1;
    char* srcs_file = "None";
    char* rcvrs_file = "None";

    double src0 [3]  = {0.0, 30.0, 0.0};
    double rcvr0 [2] = {30.0, -2.5};
    
    double theta_min = 0.5, theta_max = 45.0, az_err_lim=2.0,  freq = 0.1;
    int bnc_min = 0, bnc_max = 0, iterations=25, file_check;
    char* prof_format = "zTuvdp";
    char* topo_file = "None";
    char* output_id;
    char input_check;

    bool seq_srcs = true, write_atmo = false, custom_output_id=false;
    double seq_dr = 0.0, seq_dr_max = 5.0;
    double seq_srcs_ref [3] = {src0[0], src0[1], src0[2]};
    
    vector<double> prev_incl;
    vector<double> prev_az;
    vector<int> prev_bncs;
    
    topo::z0 = 0.0;
    atmo::tweak_abs = 1.0;
    
    geoac::is_topo = false;
    geoac::verbose = false;
       
    for(int i = 5; i < count; i++){
        if (strncmp(inputs[i], "prof_format=", 12) == 0){       prof_format = inputs[i] + 12;}
        else if (strncmp(inputs[i], "z_grnd=", 7) == 0){        topo::z0 = atof(inputs[i] + 7);}
        else if (strncmp(inputs[i], "topo_file=", 10) == 0){    topo_file = inputs[i] + 10; geoac::is_topo=true;}
    }
    
    if (geoac::is_topo){    file_check = set_region(inputs[2], inputs[3], inputs[4], topo_file, prof_format, false, world_rank);}
    else{                   file_check = set_region(inputs[2], inputs[3], inputs[4], prof_format, false, world_rank);}
    if(!file_check){
        MPI_Finalize();
        return;
    }

    if (geoac::is_topo){
        src0[1] = (geoac::lat_max + geoac::lat_min) / 2.0 * (180.0 / Pi);    rcvr0[0] = src0[1] + 0.0;
        src0[2] = (geoac::lon_max + geoac::lon_min) / 2.0 * (180.0 / Pi);    rcvr0[1] = src0[2] + 2.5;
    }
    
    for(int i = 5; i < count; i++){
        if ((strncmp(inputs[i], "incl_min=", 9) == 0) || (strncmp(inputs[i], "min_incl=", 9) == 0)){                    theta_min = atof(inputs[i] + 9);}
        else if ((strncmp(inputs[i], "incl_max=", 9) == 0) || (strncmp(inputs[i], "max_incl=", 9) == 0)){               theta_max = atof(inputs[i] + 9);}
        else if ((strncmp(inputs[i], "bnc_min=", 8) == 0) || (strncmp(inputs[i], "bnc_min=", 8) == 0)){                 bnc_min = atoi(inputs[i] + 8);}
        else if ((strncmp(inputs[i], "bnc_max=", 8) == 0) || (strncmp(inputs[i], "bnc_max=", 8) == 0)){                 bnc_max = atoi(inputs[i] + 8);}
        else if (strncmp(inputs[i], "bounces=", 8) == 0){                                                               bnc_min = atoi(inputs[i] + 8);
                                                                                                                        bnc_max = atoi(inputs[i] + 8);}
        
        else if ((strncmp(inputs[i], "src_alt=", 8) == 0) || (strncmp(inputs[i], "alt_src=", 8) == 0)){                 src0[0] = atof(inputs[i] + 8);}
        else if ((strncmp(inputs[i], "src_lat=", 8) == 0) || (strncmp(inputs[i], "lat_src=", 8) == 0)){                 src0[1] = atof(inputs[i] + 8);}
        else if ((strncmp(inputs[i], "src_lon=", 8) == 0) || (strncmp(inputs[i], "lon_src=", 8) == 0)){                 src0[2] = atof(inputs[i] + 8);}
        else if ((strncmp(inputs[i], "rcvr_lat=", 9) == 0) || (strncmp(inputs[i], "lat_rcvr=", 9) == 0)){               rcvr0[0] = atof(inputs[i] + 9);}
        else if ((strncmp(inputs[i], "rcvr_lon=", 9) == 0) || (strncmp(inputs[i], "lon_rcvr=", 9) == 0)){               rcvr0[1] = atof(inputs[i] + 9);}
        
        else if ((strncmp(inputs[i], "srcs_list=", 10) == 0) || (strncmp(inputs[i], "list_srcs=", 10) == 0)){           srcs_file = inputs[i] + 10;}
        else if ((strncmp(inputs[i], "rcvrs_list=", 11) == 0) || (strncmp(inputs[i], "list_rcvrs=", 11) == 0)){         rcvrs_file = inputs[i] + 11;}
        
        else if (strncmp(inputs[i], "seq_srcs=", 9) == 0){                                                              seq_srcs = string2bool(inputs[i] + 9);}
        else if (strncmp(inputs[i], "seq_srcs_dr=", 12) == 0){                                                          seq_dr_max = atof(inputs[i] + 12);}
        
        else if (strncmp(inputs[i], "verbose=", 8) == 0){                                                               geoac::verbose = string2bool(inputs[i] + 8);}
        else if (strncmp(inputs[i], "verbose_rcvr=", 13) == 0){                                                         geoac::verbose_opt = atof(inputs[i] + 13);}
        
        else if (strncmp(inputs[i], "iterations=", 11) == 0){                                                           iterations=atof(inputs[i] + 11);}
        else if (strncmp(inputs[i], "damping=", 8) == 0){                                                               geoac::damping = atof(inputs[i] + 8);}
        else if (strncmp(inputs[i], "tolerance=", 10) == 0){                                                            geoac::tolerance = atof(inputs[i] + 10);}
        else if (strncmp(inputs[i], "az_dev_lim=", 11) == 0){                                                           az_err_lim = atof(inputs[i] + 11);}
        else if ((strncmp(inputs[i], "incl_step_min=", 14) == 0) || (strncmp(inputs[i], "min_incl_step=", 14) == 0)){   geoac::dth_sml = atof(inputs[i] + 14) * (Pi / 180.0);}
        else if ((strncmp(inputs[i], "incl_step_max=", 14) == 0) || (strncmp(inputs[i], "max_incl_step=", 14) == 0)){   geoac::dth_big = atof(inputs[i] + 14) * (Pi / 180.0);}
        
        else if (strncmp(inputs[i], "freq=", 5) == 0){                                                                  freq = atof(inputs[i] + 5);}
        else if (strncmp(inputs[i], "abs_coeff=", 10) == 0){                                                            atmo::tweak_abs = max(0.0, atof(inputs[i] + 10));}
        
        else if ((strncmp(inputs[i], "max_alt=", 8) == 0) || (strncmp(inputs[i], "alt_max=", 8) == 0)){                 geoac::alt_max = atof(inputs[i] + 8);               if (world_rank == 0){cout << '\t' << "User selected altitude maximum = " << geoac::alt_max << '\n';}}
        else if ((strncmp(inputs[i], "max_rng=", 8) == 0) || (strncmp(inputs[i], "rng_max=", 8) == 0)){                 geoac::rng_max = atof(inputs[i] + 8);               if (world_rank == 0){cout << '\t' << "User selected range maximum = " << geoac::rng_max << '\n';}}
        else if ((strncmp(inputs[i], "max_time=", 9) == 0) || (strncmp(inputs[i], "time_max=", 9) == 0)){               geoac::time_max = atof(inputs[i] + 9);              if (world_rank == 0){cout << '\t' << "User selected propagation time maximum = " << geoac::time_max << '\n';}}

        else if ((strncmp(inputs[i], "min_lat=", 8) == 0) || (strncmp(inputs[i], "lat_min=", 8) == 0)){                 geoac::lat_min = atof(inputs[i] + 8) * Pi / 180.0;  if (world_rank == 0){cout << '\t' << "User selected latitude minimum = " << geoac::lat_min * (180.0 / Pi) << '\n';}}
        else if ((strncmp(inputs[i], "max_lat=", 8) == 0) || (strncmp(inputs[i], "lat_max=", 8) == 0)){                 geoac::lat_max = atof(inputs[i] + 8) * Pi / 180.0;  if (world_rank == 0){cout << '\t' << "User selected latitude maximum = " << geoac::lat_max * (180.0 / Pi) << '\n';}}
        else if ((strncmp(inputs[i], "min_lon=", 8) == 0) || (strncmp(inputs[i], "lon_min=", 8) == 0)){                 geoac::lon_min = atof(inputs[i] + 8) * Pi / 180.0;  if (world_rank == 0){cout << '\t' << "User selected longitude minimum = " << geoac::lon_min * (180.0 / Pi) << '\n';}}
        else if ((strncmp(inputs[i], "max_lon=", 8) == 0) || (strncmp(inputs[i], "lon_max=", 8) == 0)){                 geoac::lon_max = atof(inputs[i] + 8) * Pi / 180.0;  if (world_rank == 0){cout << '\t' << "User selected longitude maximum = " << geoac::lon_max * (180.0 / Pi) << '\n';}}
        
        else if ((strncmp(inputs[i], "min_ds=", 7) == 0) || (strncmp(inputs[i], "ds_min=", 7) == 0)){                   geoac::ds_min = atof(inputs[i] + 7);                if (world_rank == 0){cout << '\t' << "User selected ds minimum = " << geoac::ds_min << '\n';}}
        else if ((strncmp(inputs[i], "max_ds=", 7) == 0) || (strncmp(inputs[i], "ds_max=", 7) == 0)){                   geoac::ds_max = atof(inputs[i] + 7);                if (world_rank == 0){cout << '\t' << "User selected ds maximum = " << geoac::ds_max << '\n';}}
        else if ((strncmp(inputs[i], "max_s=", 6) == 0) || (strncmp(inputs[i], "s_max=", 6) == 0)){                     geoac::s_max = atof(inputs[i] + 6);                 if(world_rank == 0){cout << '\t' << "User selected s maximum (path length between reflections) = " << geoac::s_max << '\n';}}
        
        else if (strncmp(inputs[i], "prof_format=",12) == 0){                                                           prof_format = inputs[i] + 12;}
        else if (strncmp(inputs[i], "output_id=", 10) == 0){                                                            custom_output_id = true; 
                                                                                                                        output_id = inputs[i] + 10;}
        else if (strncmp(inputs[i], "z_grnd=", 7) == 0){                                                                if(!geoac::is_topo){
                                                                                                                            topo::z0 = atof(inputs[i] + 7);
                                                                                                                            topo::z_max = topo::z0;
                                                                                                                            topo::z_bndlyr = topo::z0 + 2.0;
                                                                                                                        } else {
                                                                                                                            cout << '\t' << "Note: cannot adjust ground elevation with topography." << '\n';
                                                                                                                        }}
        else if (strncmp(inputs[i], "topo_file=", 10) == 0){                                                            topo_file = inputs[i] + 10; geoac::is_topo=true;}
        else if (strncmp(inputs[i], "topo_use_BLw=", 13) == 0){                                                         topo::use_BLw = string2bool(inputs[i] + 13);}
        else{
            if (world_rank == 0){
                cout << "***WARNING*** Unrecognized parameter entry: " << inputs[i] << '\n';
            }
            MPI_Finalize();
            return;
        }
    }

    // set up source(s) and receiver(s)
    double** srcs;
    double** rcvrs;
    
    ifstream file_in;
    // Read in source(s) or use reference for single
    if (strncmp(srcs_file, "None", 4) == 0){
        srcs = new double * [srcs_cnt];
        
        srcs[0] = new double [3];
        srcs[0][0] = src0[0];
        srcs[0][1] = src0[1];
        srcs[0][2] = src0[2];
    } else{
        if(world_rank == 0){
            srcs_cnt = file_length(srcs_file);
        }
        MPI_Bcast(&srcs_cnt, 1, MPI_INT, 0, MPI_COMM_WORLD);
        MPI_Barrier(MPI_COMM_WORLD);
        
        srcs = new double * [srcs_cnt];
        
        if(world_rank == 0){
            file_in.open(srcs_file);
        }
        for(int n = 0; n < srcs_cnt; n++){
            if(world_rank == 0){
                file_in >> src0[1] >> src0[2] >> src0[0];
            }
            MPI_Bcast(src0, 3, MPI_DOUBLE, 0, MPI_COMM_WORLD);
            MPI_Barrier(MPI_COMM_WORLD);
            
            srcs[n] = new double [3];
            srcs[n][0] = src0[0];
            srcs[n][1] = src0[1];
            srcs[n][2] = src0[2];
        }
        if(world_rank == 0){file_in.close();}
    }
    
    // Read in reciever(s) or use reference for single
    if (strncmp(rcvrs_file, "None", 4) == 0){
        rcvrs = new double * [rcvrs_cnt];
        
        rcvrs[0] = new double [2];
        rcvrs[0][0] = rcvr0[0];
        rcvrs[0][1] = rcvr0[1];
    } else{
        if(world_rank == 0){rcvrs_cnt = file_length(rcvrs_file);}
        MPI_Bcast(&rcvrs_cnt, 1, MPI_INT, 0, MPI_COMM_WORLD);
        MPI_Barrier(MPI_COMM_WORLD);
        
        rcvrs = new double * [rcvrs_cnt];
        
        if(world_rank == 0){file_in.open(rcvrs_file);}
        for(int n = 0; n < rcvrs_cnt; n++){
            if(world_rank == 0){file_in >> rcvr0[0] >> rcvr0[1];}
            MPI_Bcast(rcvr0, 2, MPI_DOUBLE, 0, MPI_COMM_WORLD);
            MPI_Barrier(MPI_COMM_WORLD);
            
            rcvrs[n] = new double [2];
            rcvrs[n][0] = rcvr0[0];
            rcvrs[n][1] = rcvr0[1];
        }
        if(world_rank == 0){file_in.close();}
    }
    
    if (rcvrs_cnt > world_size){
        if (world_rank == 0){
            cout << "***WARNING*** Number of receivers exceeds cpu count!  Subdivide receiver list for optimal analysis efficiency." << '\n';
        }
        MPI_Finalize();
        return;
    }

    if(world_rank == 0 && write_atmo){
        geoac::write_prof("atmo.dat", srcs[0][1], srcs[0][2], globe::bearing(srcs[0][1], srcs[0][2], rcvrs[geoac::verbose_opt][0], rcvrs[geoac::verbose_opt][1]));
    }
        
    // modify units (degrees -> radians)
    theta_min *= (Pi / 180.0);
    theta_max *= (Pi / 180.0);
    
    for(int n = 0; n < srcs_cnt; n++){
        srcs[n][1] *= (Pi / 180.0);
        srcs[n][2] *= (Pi / 180.0);
        srcs[n][0] = max(srcs[n][0], topo::z(srcs[n][1], srcs[n][2]) - globe::r0);
    }
    for(int n = 0; n < rcvrs_cnt; n++){
        rcvrs[n][0] *= (Pi / 180.0);
        rcvrs[n][1] *= (Pi / 180.0);
    }
    
    // display run summary
    if(world_rank == 0){
        cout << '\n' << "Parameter summary:" << '\n';
        for(int n = 0; n < min(srcs_cnt, 10); n++){
            cout << '\t' << '\t' << "(" << srcs[n][1] * (180.0 / Pi) << ", " << srcs[n][2] * (180.0 / Pi) << ", " << srcs[n][0] << ")" << '\n';
        }
        if (srcs_cnt > 10){ cout << '\t' << '\t' << "..." << '\n';}
        cout << '\t' << "receiver location(s) (lat, lon, alt):" << '\n';
        for(int n = 0; n < min(rcvrs_cnt, 10); n++){
            cout << '\t' << '\t' << "(" << rcvrs[n][0] * (180.0 / Pi) << ", " << rcvrs[n][1] * (180.0 / Pi);
            cout << ", " << topo::z(rcvrs[n][0], rcvrs[n][1]) - globe::r0 <<'\n';
        }
        if (rcvrs_cnt > 10){
            cout << '\t' << '\t' << "..." << '\n';
            cout << '\t' << '\t' << "(" << rcvrs[rcvrs_cnt - 2][0] * (180.0 / Pi) << ", " << rcvrs[rcvrs_cnt - 2][1] * (180.0 / Pi);
            cout << ", " << topo::z(rcvrs[rcvrs_cnt - 2][0], rcvrs[rcvrs_cnt - 2][1]) - globe::r0 <<'\n';

            cout << '\t' << '\t' << "(" << rcvrs[rcvrs_cnt - 1][0] * (180.0 / Pi) << ", " << rcvrs[rcvrs_cnt - 1][1] * (180.0 / Pi);
            cout << ", " << topo::z(rcvrs[rcvrs_cnt - 1][0], rcvrs[rcvrs_cnt - 1][1]) - globe::r0 <<'\n';
        }
        cout << '\t' << "inclination range: " << theta_min * (180.0 / Pi) << ", " << theta_max * (180.0 / Pi) << '\n';
        cout << '\t' << "bounces: " << bnc_min << ", " << bnc_max << '\n';
        if(!geoac::is_topo){
            cout << '\t' << "ground elevation: " << topo::z0 << '\n';
        }
        cout << '\t' << "damping: " << geoac::damping << '\n';
        cout << '\t' << "frequency: " << freq << '\n';
        cout << '\t' << "S&B atten coeff: " << atmo::tweak_abs << '\n';
        if (geoac::verbose){
            cout << '\t' << "verbose: true" << '\n';
        } else {
            cout << '\t' << "verbose: false" << '\n';
        }
        cout << '\t' << "threads: " << world_size << '\n' << '\n';
    }
    MPI_Barrier(MPI_COMM_WORLD);
    
    // define the tasking break down to distribution recievers
    task_id = define_tasking(rcvrs_cnt, world_rank, world_size, group_rank, group_size, false);

    // Extract the file name from the input and use it to distinguish the resulting output
    char output_buffer [512];
    if(!custom_output_id){
        output_id = inputs[2];
    }

    
    double theta_start, theta_next, theta_est, phi_est;
    bool estimate_success;
    
    for(int n_src = 0; n_src < srcs_cnt; n_src++){
        geoac::eigenray_cnt = 0;
        if (world_rank == 0){
            cout << '\n' << "Searching for eigenrays from source at " << srcs[n_src][1] * (180.0 / Pi) << ", " << srcs[n_src][2] * (180.0 / Pi) << ", " << srcs[n_src][0] << '\n';
        }
        
        if (group_rank == 0){
            if(srcs_cnt == 1 && rcvrs_cnt == 1){
                sprintf(output_buffer, "%s.arrivals.dat", output_id);
            } else if (srcs_cnt == 1 && rcvrs_cnt > 1) {
                sprintf(output_buffer, "%s.rcvr-%i.arrivals.dat", output_id, task_id);
            } else if (srcs_cnt > 1 && rcvrs_cnt == 1) {
                sprintf(output_buffer, "%s.src-%i.arrivals.dat", output_id, n_src);
            } else {
                sprintf(output_buffer, "%s.src-%i.rcvr-%i.arrivals.dat", output_id, n_src, task_id);
            }
            geoac::eig_results.open(output_buffer);
            
            geoac::eig_results << "# infraga-accel-sph-rngdep -eig_search summary:" << '\n';
            geoac::eig_results << "# " << '\t' << "profile prefix: " << inputs[2] << '\n';
            geoac::eig_results << "# " << '\t' << "profile grid files: " << inputs[3] << ", " << inputs[4] << '\n';
            geoac::eig_results << "# " << '\t' << "source location (lat, lon, alt): " << srcs[n_src][1] * (180.0 / Pi) << ", " << srcs[n_src][2] * (180.0 / Pi) << ", " << srcs[n_src][0] << '\n';
            geoac::eig_results << "# " << '\t' << "receiver location (lat, lon, alt): " << rcvrs[task_id][0] * (180.0 / Pi) << ", " << rcvrs[task_id][1] * (180.0 / Pi) << ", " << topo::z(rcvrs[task_id][0], rcvrs[task_id][1]) - globe::r0 << '\n';
            geoac::eig_results << "# " << '\t' << "inclination range: " << theta_min * (180.0 / Pi) << ", " << theta_max * (180.0 / Pi) << '\n';
            geoac::eig_results << "# " << '\t' << "bounces: " << bnc_min << ", " << bnc_max << '\n';
            if(!geoac::is_topo){
                geoac::eig_results << "#" << '\t' << "ground elevation: " << topo::z0 << '\n';
            } else {
                geoac::eig_results << "#" << '\t' << "topo file:" << topo_file << '\n';
            }
            geoac::eig_results << "# " << '\t' << "damping: " << geoac::damping << '\n';
            geoac::eig_results << "# " << '\t' << "frequency: " << freq << '\n';
            geoac::eig_results << "# " << '\t' << "S&B atten coeff: " << atmo::tweak_abs << '\n';
            geoac::eig_results << "# " << '\t' << "threads: " << world_size << '\n' << '\n';
                    
            geoac::eig_results << "# incl [deg]";
            geoac::eig_results << '\t' << "az [deg]";
            geoac::eig_results << '\t' << "n_b";
            geoac::eig_results << '\t' << "lat_0 [deg]";
            geoac::eig_results << '\t' << "lon_0 [deg]";
            geoac::eig_results << '\t' << "time [s]";
            geoac::eig_results << '\t' << "cel [km/s]";
            geoac::eig_results << '\t' << "turning ht [km]";
            geoac::eig_results << '\t' << "inclination [deg]";
            geoac::eig_results << '\t' << "back azimuth [deg]";
            geoac::eig_results << '\t' << "trans. coeff. [dB]";
            geoac::eig_results << '\t' << "absorption [dB]";
            geoac::eig_results << '\n';
                    
            if(srcs_cnt == 1 && rcvrs_cnt == 1){
                sprintf(output_buffer, "%s.eigenray-", output_id);
            } else if (srcs_cnt == 1 && rcvrs_cnt > 1) {
                sprintf(output_buffer, "%s.rcvr-%i.eigenray-", output_id, task_id);
            } else if (srcs_cnt > 1 && rcvrs_cnt == 1) {
                sprintf(output_buffer, "%s.src-%i.eigenray-", output_id, n_src);
            } else {
                sprintf(output_buffer, "%s.src-%i.rcvr-%i.eigenray-", output_id, n_src, task_id);
            }
                    
        }
        MPI_Barrier(MPI_COMM_WORLD);
        
        // perform the actual analysis
        for(int n_rcvr = 0; n_rcvr < rcvrs_cnt; n_rcvr++){
            if(task_id == n_rcvr){
                if(seq_srcs){
                    seq_dr = sqrt(pow(globe::gc_dist(srcs[n_src][1], srcs[n_src][2], seq_srcs_ref[1], seq_srcs_ref[2]), 2)
                                    + pow(srcs[n_src][0] - seq_srcs_ref[0], 2));

                    if (n_src == 0 || seq_dr > seq_dr_max){
                        // Complete full search and store eigenrays to previous vectors...
                        for(int j = 0; j < 3; j++){
                            seq_srcs_ref[j] = srcs[n_src][j];
                        }

                        prev_incl.clear();
                        prev_az.clear();
                        prev_bncs.clear();

                        for(int n_bnc = bnc_min; n_bnc <= bnc_max; n_bnc++){
                            theta_start = theta_min;
                            while(theta_start < theta_max){
                                estimate_success = geoac::est_eigenray(srcs[n_src], rcvrs[n_rcvr], theta_start, theta_max, theta_est, phi_est, theta_next, n_bnc, az_err_lim, task_comms[n_rcvr], group_rank, group_size, task_id);
                                if(estimate_success && group_rank == 0){
                                    if(geoac::find_eigenray(srcs[n_src], rcvrs[n_rcvr], theta_est, phi_est, freq, n_bnc, iterations, output_buffer, n_rcvr)){
                                        prev_incl.push_back(theta_est);
                                        prev_az.push_back(phi_est);
                                        prev_bncs.push_back(n_bnc);
                                    }
                                }
                                MPI_Barrier(task_comms[n_rcvr]);
                                theta_start = theta_next;
                            }
                        }
                    } else {
                        // Or perturb previous the previous solution
                        if(group_rank == 0){
                            for(int n_prev = 0; n_prev < prev_incl.size(); n_prev++){
                                geoac::find_eigenray(srcs[n_src], rcvrs[n_rcvr], prev_incl[n_prev], prev_az[n_prev], freq, prev_bncs[n_prev], iterations, output_buffer, n_rcvr);
                            }
                        }
                        MPI_Barrier(task_comms[n_rcvr]);
                    }
                } else {
                    for(int n_bnc = bnc_min; n_bnc <= bnc_max; n_bnc++){
                        theta_start = theta_min;
                        while(theta_start < theta_max){
                            estimate_success = geoac::est_eigenray(srcs[n_src], rcvrs[n_rcvr], theta_start, theta_max, theta_est, phi_est, theta_next, n_bnc, az_err_lim, task_comms[n_rcvr], group_rank, group_size, task_id);
                            if(estimate_success && group_rank == 0){
                                geoac::find_eigenray(srcs[n_src], rcvrs[n_rcvr], theta_est, phi_est, freq, n_bnc, iterations, output_buffer, n_rcvr);
                            }
                            MPI_Barrier(task_comms[n_rcvr]);
                            theta_start = theta_next;
                        }
                    }
                }
            }
        }
        MPI_Barrier(MPI_COMM_WORLD);

                
        if (group_rank == 0){
            geoac::eig_results.close();
        }
        for(int n_rcvr = 0; n_rcvr < rcvrs_cnt; n_rcvr++){
            if(group_rank == 0 && n_rcvr == task_id){
                cout << '\t' << "Identified " << geoac::eigenray_cnt << " eigenray(s) to receiver at " << rcvrs[task_id][0] * (180.0 / Pi);
                cout << ", " << rcvrs[task_id][1] * (180.0 / Pi) << ", " << topo::z(rcvrs[task_id][0], rcvrs[task_id][1]) - globe::r0 << '\n';
            }
        }
        MPI_Barrier(MPI_COMM_WORLD);
    }
    MPI_Barrier(MPI_COMM_WORLD);
            
    for(int n = 0; n < srcs_cnt; n++){ delete srcs[n];}
    for(int n = 0; n < rcvrs_cnt; n++){ delete rcvrs[n];}
    delete [] srcs;
    delete [] rcvrs;
    
    clear_region();
    free_tasking(rcvrs_cnt);
    MPI_Finalize();
}

int main(int argc, char* argv[]){
    if (argc < 5){ usage();}
    else {
        if ((strncmp(argv[1], "-version", 8) == 0) || (strncmp(argv[1], "-v", 2) == 0)){            version();}
        else if ((strncmp(argv[1], "-usage", 6) == 0) || (strncmp(argv[1], "-u", 2) == 0)){         usage();}
        else if ((strncmp(argv[1], "-prop", 5) == 0)|| (strncmp(argv[1], "-p", 2) == 0)){           run_prop(argv, argc);}
        else if ((strncmp(argv[1], "-eig_search", 11) == 0)|| (strncmp(argv[1], "-s", 2) == 0)){    run_eig_search(argv, argc);}
        else {                                                                                      cout << "Unrecognized option." << '\n';}
    }
    return 0;}
