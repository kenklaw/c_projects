#include "v3math.h"
#include "ppmrw.h"

const int MAX_SIZE = 128;

enum shape_type{Sphere, Plane}; // 0 = sphere, 1 = plane

typedef struct {

    enum shape_type type;
    int diffuse_color[3];
    int specular_color[3];
    float center[3];
    float reflectivity;
    int texture_index;

    union {

        struct {
            float radius;
        } sphere;

        struct {
            float normal[3];
        } plane;
    };
} object;

typedef struct {

    // theta of 0 = point light.
    float theta;
    float color[3];
    float center[3];
    float radial[3];

    // Only for spot lights (theta != 0)
    float angular_a0;
    float direction[3];
    float cosine;
    
} light;

typedef struct {

    int width;
    int height;
    uint8_t *pixmap;

} texture;

void raytrace_fail(char *s) {

    printf("Error: %s\n\n", s);
    printf("Usage:\n");
    printf("raytrace WIDTH HEIGHT INPUT.scene OUTPUT.ppm\n\n");
    exit(1);
}

// Gets the camera width and height and stores them in the provided pointers.
// Returns the file pointer pointing to the first object in the list.
FILE *get_camera(FILE *fp, float *camera_width, float *camera_height) {

    char string_width[MAX_SIZE];
    char string_height[MAX_SIZE];

    // Assuming the camera will always be formatted correctly,
    // we can use an fscanf pattern for parsing the width and height.
    fscanf(fp, "%*s %*s %s %*s %s", string_width, string_height);
    float width = atof(string_width);
    float height = atof(string_height);

    *camera_width = width;
    *camera_height = height;

    return fp;
}

void get_objects(FILE *fp, object *object_list, light *light_list, texture *texture_list, 
    int *num_objects, int *num_lights, int *num_textures) {

    char string_buffer[MAX_SIZE];

    int object_index = 0;
    int light_index = 0;
    int iterations = 0;
    int texture_index = 0;

    while(!feof(fp)){

        // Prevents infinite loops caused by improper formatting and
        // prevents segfaults from adding too many objects + lights. (>128)
        if(iterations >= 128){
        printf("\nError: Improper formatting of scene file or too many items listed.");
        exit(1);
        }

        // Grab the object's type.
        fscanf(fp, " %128[^,] ", string_buffer);

        if(strcmp(string_buffer, "sphere") == 0 || strcmp(string_buffer, "plane") == 0) {

            object new_object;

            // Default the diffuse color to black in case a texture is used instead.
            new_object.diffuse_color[0] = 0;
            new_object.diffuse_color[1] = 0;
            new_object.diffuse_color[2] = 0;

            // Default the texture index to -1 in case a texture is not used.
            new_object.texture_index = -1;

            // Default the specular color to black in case it isn't listed.
            new_object.specular_color[0] = 0;
            new_object.specular_color[1] = 0;
            new_object.specular_color[2] = 0;

            if(strcmp(string_buffer, "sphere") == 0) {

                new_object.type = Sphere;

            }

            if(strcmp(string_buffer, "plane") == 0) {

                new_object.type = Plane;

            }

            // prime the while-loop.
            char delim = fgetc(fp);

            // iterate until the end of the line
            while(delim == ','){

                fscanf(fp, " %s ", string_buffer);

                if(strcmp(string_buffer, "radius:") == 0){

                    fscanf(fp, " %128[^,^\n]", string_buffer);

                    new_object.sphere.radius = atof(string_buffer);

                }

                else if(strcmp(string_buffer, "diffuse_color:") == 0){

                    fscanf(fp, " [%s ", string_buffer);

                    new_object.diffuse_color[0] = floor(255 * atof(string_buffer));

                    fscanf(fp, " %s", string_buffer);

                    new_object.diffuse_color[1] = floor(255 * atof(string_buffer));

                    fscanf(fp, " %128[^,^\n]", string_buffer);

                    new_object.diffuse_color[2] =  floor(255 * atof(string_buffer));

                }

                else if(strcmp(string_buffer, "specular_color:") == 0){

                    fscanf(fp, " [%s ", string_buffer);

                    new_object.specular_color[0] = floor(255 * atof(string_buffer));

                    fscanf(fp, " %s", string_buffer);

                    new_object.specular_color[1] = floor(255 * atof(string_buffer));

                    fscanf(fp, " %128[^,^\n]", string_buffer);

                    new_object.specular_color[2] = floor(255 * atof(string_buffer));

                }

                else if(strcmp(string_buffer, "position:") == 0){

                    fscanf(fp, " [%s ", string_buffer);

                    new_object.center[0] = atof(string_buffer);

                    fscanf(fp, " %s", string_buffer);

                    new_object.center[1] = atof(string_buffer);

                    fscanf(fp, " %128[^,^\n]", string_buffer);

                    new_object.center[2] = atof(string_buffer);

                }
                
                else if(strcmp(string_buffer, "normal:") == 0){

                    fscanf(fp, " [%s ", string_buffer);

                    new_object.plane.normal[0] = atof(string_buffer);

                    fscanf(fp, " %s", string_buffer);

                    new_object.plane.normal[1] = atof(string_buffer);

                    fscanf(fp, " %128[^,^\n]", string_buffer);

                    new_object.plane.normal[2] = atof(string_buffer);

                }

               else if(strcmp(string_buffer, "reflectivity:") == 0){

                    fscanf(fp, " %128[^,^\n]", string_buffer);

                    new_object.reflectivity = atof(string_buffer);

                }

                else if(strcmp(string_buffer, "texture:") == 0){

                    fscanf(fp, " %128[^,^\n]", string_buffer);

                    texture new_texture;

                    // string_buffer contains the filename of the texture.
                    FILE *texture_fp = fopen(string_buffer, "r");

                    char header_num[3];

                    int max_val = 0;

                    texture_fp = read_header(texture_fp, header_num, &new_texture.width,
                                             &new_texture.height, &max_val);

                    int length = new_texture.width * new_texture.height * 3;

                    uint8_t *pixmap = malloc(sizeof(uint8_t) * length);

                    if(strcmp(header_num, "P3") == 0) {

                        read_p3(texture_fp, pixmap, length);

                    } else {

                        read_p6(texture_fp, pixmap, length);
                    }

                    new_texture.pixmap = pixmap;

                    texture_list[texture_index] = new_texture;

                    new_object.texture_index = texture_index;

                    texture_index++;

                }

                delim = fgetc(fp);
            }

            object_list[object_index] = new_object;
            object_index++;

        }

        if(strcmp(string_buffer, "light") == 0) {

            light new_light;

            // Default to 0.0 in case 
            new_light.theta = 0.0;

            // prime the while-loop.
            char delim = fgetc(fp);

            // iterate until the end of the line
            while(delim == ','){

                fscanf(fp, " %s ", string_buffer);

                if(strcmp(string_buffer, "theta:") == 0){

                    fscanf(fp, " %128[^,^\n]", string_buffer);

                    new_light.theta = atof(string_buffer);

                    // Only need to calculate cosine if the light is a spotlight.
                    if(new_light.theta != 0){

                        // convert theta to radians for cosine function
                        float rad_theta = new_light.theta * M_PI / 180;

                        new_light.cosine = cos(rad_theta);
                    }

                }

                else if(strcmp(string_buffer, "radial-a0:") == 0){

                    fscanf(fp, " %128[^,^\n]", string_buffer);

                    new_light.radial[0] = atof(string_buffer);

                }

                else if(strcmp(string_buffer, "radial-a1:") == 0){

                    fscanf(fp, " %128[^,^\n]", string_buffer);

                    new_light.radial[1] = atof(string_buffer);

                }

                else if(strcmp(string_buffer, "radial-a2:") == 0){

                    fscanf(fp, " %128[^,^\n]", string_buffer);

                    new_light.radial[2] = atof(string_buffer);

                }

                else if(strcmp(string_buffer, "angular-a0:") == 0){

                    fscanf(fp, " %128[^,^\n]", string_buffer);

                    new_light.angular_a0 = atof(string_buffer);

                }

                else if(strcmp(string_buffer, "color:") == 0){

                    fscanf(fp, " [%s ", string_buffer);

                    new_light.color[0] = atof(string_buffer);

                    fscanf(fp, " %s", string_buffer);

                    new_light.color[1] = atof(string_buffer);

                    fscanf(fp, " %128[^,^\n]", string_buffer);

                    new_light.color[2] =  atof(string_buffer);

                }

                else if(strcmp(string_buffer, "direction:") == 0){

                    fscanf(fp, " [%s ", string_buffer);

                    new_light.direction[0] = atof(string_buffer);

                    fscanf(fp, " %s", string_buffer);

                    new_light.direction[1] = atof(string_buffer);

                    fscanf(fp, " %128[^,^\n]", string_buffer);

                    new_light.direction[2] = atof(string_buffer);

                }

                else if(strcmp(string_buffer, "position:") == 0){

                    fscanf(fp, " [%s ", string_buffer);

                    new_light.center[0] = atof(string_buffer);

                    fscanf(fp, " %s", string_buffer);

                    new_light.center[1] = atof(string_buffer);

                    fscanf(fp, " %128[^,^\n]", string_buffer);

                    new_light.center[2] = atof(string_buffer);

                }

                delim = fgetc(fp);
            }

            light_list[light_index] = new_light;
            light_index++;

        }

        iterations++;
    }

    *num_objects = object_index;
    *num_lights = light_index;
    *num_textures = texture_index;
}

            
float sphere_intersection(float *rd, float *ro, float *center, float radius){
        
    float x_diff = ro[0] - center[0];
    float y_diff = ro[1] - center[1];
    float z_diff = ro[2] - center[2];      

    //float a = pow(rd[0], 2) + pow(rd[1], 2) + pow(rd[2], 2);
    float b = 2 * (rd[0] * x_diff + rd[1] * y_diff + rd[2] * z_diff);
    float c = pow(x_diff, 2) + pow(y_diff, 2) + pow(z_diff, 2);
          c = c - pow(radius, 2);

    float discrim = pow(b, 2) - 4 * c;

    if(discrim >= 0.0){

        float t0 = (-b - sqrt(discrim)) / 2;

        if(t0 > 0.0){

            return t0;

        } else {

            float t1 = (-b + sqrt(discrim)) / 2;
            return t1;
        }

    } else {
        // return -1 to indicate that there is no intersection
        return -1;
    }    
}


float plane_intersection(float *rd, float *ro, float *center, float *normal){

    float vd = v3_dot_product(normal, rd);

    if(vd > 0.0){

        return -1;

    } else {
            
        float numerator_comp[3] = {0,0,0};       
        v3_subtract(numerator_comp, center, ro);
    
        float vn = v3_dot_product(numerator_comp, normal);

        return vn / vd;   
    }
}


float radial(float a2, float a1, float a0, float d){

    float rad = a2 * (d*d) + a1*d + a0;

    if(rad == 0){

        return 0;

    }
    if(d == INFINITY){

        return 1;

    } else {

        return 1 / rad;
    }
}


float angular(light light, float *v_obj){

    if(light.theta == 0){

        return 1;

    } else {

        float dot = v3_dot_product(v_obj, light.direction);

        if(dot < light.cosine) {

            return 0;

        } else {

            return pow(dot, light.angular_a0);
        }
    }

}


void apply_lights(object *object_list, int num_objects, light *light_list, int num_lights,
                    texture *texture_list, float *intersection, float *rd, 
                    int subject_object_index, float *I){

    for(int light_index = 0; light_index < num_lights; light_index++){

        light iter_light = light_list[light_index];

        float I_comp[3] = {0, 0, 0};

        float v_obj[3];
        v3_from_points(v_obj, iter_light.center, intersection);

        v3_normalize(v_obj, v_obj);

        float lit_object_t = INFINITY;
        int lit_object_index = -1;
        float light_t = -1;

        for(int object_index = 0; object_index < num_objects; object_index++){

            object iter_object = object_list[object_index];

            if(iter_object.type == Sphere){  
            
                light_t = sphere_intersection(v_obj, iter_light.center, iter_object.center, iter_object.sphere.radius);

            }

            if(iter_object.type == Plane){
            
                light_t = plane_intersection(v_obj, iter_light.center, iter_object.center, iter_object.plane.normal);

            }

            if(light_t >= 0.0){
                
                if(light_t < lit_object_t){

                    lit_object_index = object_index;
                    lit_object_t = light_t;
                }
            }
        }

        if(subject_object_index == lit_object_index){

            object lit_object = object_list[lit_object_index];

            float light_x_diff = intersection[0] - iter_light.center[0];
            float light_y_diff = intersection[1] - iter_light.center[1];
            float light_z_diff = intersection[2] - iter_light.center[2];

            float distance = sqrt(pow(light_x_diff, 2) + pow(light_y_diff, 2) + pow(light_z_diff, 2));

            float f_rad = radial(iter_light.radial[2], iter_light.radial[1],
                iter_light.radial[0], distance);

            float normal[3] = {0, 0, 0}; 

            if(lit_object.type == Sphere){

                v3_from_points(normal, lit_object.center, v_obj);

            }

            if(lit_object.type == Plane){

                normal[0] = lit_object.plane.normal[0];
                normal[1] = lit_object.plane.normal[1];
                normal[2] = lit_object.plane.normal[2];
            }


            v3_normalize(normal, normal);

            float I_l[3] = {iter_light.color[0], iter_light.color[1], iter_light.color[2]};

            float L[3] = {v_obj[0], v_obj[1], v_obj[2]};

            v3_scale(L, -1);

            float n_dot_L = v3_dot_product(normal, L);

            v3_scale(I_l, n_dot_L);

            float diffuse_comp[3];


            if(lit_object.texture_index == -1) {

                diffuse_comp[0] = lit_object.diffuse_color[0] * I_l[0];
                diffuse_comp[1] = lit_object.diffuse_color[1] * I_l[1];
                diffuse_comp[2] = lit_object.diffuse_color[2] * I_l[2];

            } else {

                texture obj_texture = texture_list[lit_object.texture_index];

                float u;
                float v;

                if(lit_object.type == Sphere) {

                    float theta = atan2(-(intersection[2] - lit_object.center[2]), 
                                        intersection[0] - lit_object.center[0]);
                    u = (theta + M_PI) / (2 * M_PI);
                    float fi = acos((-(intersection[1] - lit_object.center[1])) / lit_object.sphere.radius);
                    v = fi / M_PI;

                    //printf("\n(sphere)u: %f, v: %f", u, v);

                    u = u * obj_texture.width;
                    v = v * obj_texture.height;

                } else {

                    float vector_v[3] = {0, 1, 1};

                    float vector_u[3] = {0, 0, 0};

                    v3_cross_product(vector_u, vector_v, lit_object.plane.normal);

                    u = v3_dot_product(intersection, vector_u);
                    v = v3_dot_product(intersection, vector_v);

                    u = remainder(u, 70.0);
                    v = remainder(v, 50.0);

                    u = u * 50;
                    v = v * 50;

                    //printf("\n(plane) TC: %d", texture_coord);

                }


                float y_offset = floor(obj_texture.height - v) * obj_texture.width;
                int texture_coord = (int) (y_offset * 3 + floor(u) * 3);


                diffuse_comp[0] = obj_texture.pixmap[texture_coord] * I_l[0];
                diffuse_comp[1] = obj_texture.pixmap[texture_coord + 1] * I_l[1];
                diffuse_comp[2] = obj_texture.pixmap[texture_coord + 2] * I_l[2];
            }

            // Using a second I_l to account for scaling mutations.
            float I_l_2[3];
            I_l_2[0] = iter_light.color[0];
            I_l_2[1] = iter_light.color[1];
            I_l_2[2] = iter_light.color[2];

            float R[3];

            v3_reflect(R, L, normal);

            float V[3] = {rd[0], rd[1], rd[2]};

            v3_scale(V, -1);

            float R_dot_V = v3_dot_product(R, V);

            // Hard-coded ns to be 20
            R_dot_V = pow(R_dot_V, 20);

            v3_scale(I_l_2, R_dot_V);

            float specular_comp[3];

            specular_comp[0] = lit_object.specular_color[0] * I_l_2[0];
            specular_comp[1] = lit_object.specular_color[1] * I_l_2[1];
            specular_comp[2] = lit_object.specular_color[2] * I_l_2[2];

            v3_add(I_comp, diffuse_comp, specular_comp);

            v3_scale(I_comp, f_rad);

            float f_ang = angular(iter_light, v_obj);

            v3_scale(I_comp, f_ang);

            // Accrue the I component into I for each light.
            v3_add(I, I, I_comp);
        }
    }
}


void reflection(object *object_list, int num_objects, light *light_list, int num_lights,
                    float *intersection, float *rd, int current_object_index, 
                    int level, float *returned_color, texture *texture_list){

    object current_object = object_list[current_object_index];

    // Reflected color begins as pure black.
    float reflected_color[3] = {0, 0, 0};


    // Stop recursion after 7 levels of recursion.
    if(level <= 7){

        float normal[3]; 

        if(current_object.type == Sphere){

            v3_from_points(normal, current_object.center, intersection);

        }

        if(current_object.type == Plane){

            normal[0] = current_object.plane.normal[0];
            normal[1] = current_object.plane.normal[1];
            normal[2] = current_object.plane.normal[2];
        }

        v3_normalize(normal, normal);
        
        float reflection_vector[3];
        v3_reflect(reflection_vector, rd, normal);

        float t = -1;
        float smallest_t = INFINITY;
        int closest_to_object_index = -1;

        // Find the closest object hit by the reflected ray, if it exists.
        for(int object_index = 0; object_index < num_objects; object_index++){
            
            object iter_object = object_list[object_index];

            // Skip over the current object to prevent floating point errors.
            if(object_index != current_object_index){

                if(iter_object.type == Sphere){  
                    
                    t = sphere_intersection(reflection_vector, intersection, iter_object.center, iter_object.sphere.radius);
                }

                if(iter_object.type == Plane){
                    
                    t = plane_intersection(reflection_vector, intersection, iter_object.center, iter_object.plane.normal); 
                }

                if(t >= 0.0){

                    if(t < smallest_t){

                        smallest_t = t;
                        closest_to_object_index = object_index;
                    }
                }
            }
        }

        // Check for an intersection.
        if(smallest_t < INFINITY){

            // Calculate the intersection of the reflected ray.
            float new_intersection[3] = {reflection_vector[0], reflection_vector[1], reflection_vector[2]};
            v3_scale(new_intersection, smallest_t);
            v3_add(new_intersection, new_intersection, intersection);

            // Since there is an intersection, recurse.
            reflection(object_list, num_objects, light_list, num_lights,
                        new_intersection, reflection_vector, closest_to_object_index, 
                        level + 1, reflected_color, texture_list);

        }

        float I[3] = {0, 0, 0};

        // Apply all lights to the current object.
        apply_lights(object_list, num_objects, light_list, num_lights, texture_list,
            intersection, rd, current_object_index, I);

        // Calculate the opaque color.
        v3_scale(I, 1.0 - current_object.reflectivity);

        // Apply the reflectivity to the reflected color.
        v3_scale(reflected_color, current_object.reflectivity);

        // Add the opaque color to the reflected color and store it in
        // returned_color
        v3_add(returned_color, I, reflected_color);
    }
}


int clamp(int color){

    if(color > 255){

        color = 255;

    }

    if(color < 0){

        color = 0;

    }

    return color;
}


// Takes in all of the information provided by the input file
// and performs the raytraceing algorithm to generate an image that is then 
// stored in the given pixmap.
void raytrace(uint8_t *pixmap, float cam_width, float cam_height, object *object_list,
                int num_objects, light *light_list, int num_lights, float user_width, 
                float user_height, texture *texture_list) {
    
    // We assume the camera postion is at [0, 0, 0]
    float camera_position[] = {0,0,0};

    float viewplane_x;
    float viewplane_y;
    float viewplane_z = -1;

    // This is how large each "window" will be in the frame
    float pixwidth = cam_width / user_width;
    float pixheight = cam_height / user_height;

    int pixmap_index = 0;

    // The camera is aimed directly down the z-axis.
    float cam_center_x = 0;
    float cam_center_y = 0;

    for(int row_index = 0; row_index < user_height; row_index++){

        // y coordinate of viewplane row
        viewplane_y  = cam_center_y - (cam_height / 2) + (pixheight * (row_index + 0.5));

        for(int col_index = 0; col_index < user_width; col_index++){

            // x coordinate of viewplane column
            viewplane_x = cam_center_x - (cam_width / 2) + (pixwidth * (col_index + 0.5));

            // direction vector without t scalar
            // viewplane_y is negated to account for ppm writer that writes
            // in the negative-y direction.
            float rd[3] = {viewplane_x, -viewplane_y, viewplane_z};
            
            v3_normalize(rd, rd);

            object *closest_object = NULL;
            float closest_to_camera_t = INFINITY;
            int closest_to_camera_index;
            float t;
            
            for(int object_index = 0; object_index < num_objects; object_index++){
                
                object iter_object = object_list[object_index];

                if(iter_object.type == Sphere){  
                    
                    t = sphere_intersection(rd, camera_position, iter_object.center, iter_object.sphere.radius);
                }

                if(iter_object.type == Plane){
                    
                    t = plane_intersection(rd, camera_position, iter_object.center, iter_object.plane.normal); 
                }

                if(t >= 0.0){
                    if(t < closest_to_camera_t){
                        closest_object = &object_list[object_index];
                        closest_to_camera_t = t;
                        closest_to_camera_index = object_index;
                    }
                }

            }

            float color[3] = {0, 0, 0};

            if(closest_object != NULL){

                float intersection[3] = {rd[0], rd[1], rd[2]};
                v3_scale(intersection, closest_to_camera_t);
                v3_add(intersection, intersection, camera_position);

                reflection(object_list, num_objects, light_list, num_lights,
                        intersection, rd, closest_to_camera_index, 0, color, texture_list);
            }


            pixmap[pixmap_index] = clamp((int) color[0]); // write R
            pixmap[pixmap_index + 1] = clamp((int) color[1]); // write G
            pixmap[pixmap_index + 2] = clamp((int) color[2]); // write B

            pixmap_index+=3;
        }
    }
}


int main( int argc, char *argv[] ){

    // Check to make sure there are enough arguments in the CLI
    if (argc != 5) {
        raytrace_fail( "Wrong number of arguments." );
    }

    // Retrieve the arguments from the CLI
    float width = atof(argv[1]);
    float height = atof(argv[2]);
    char *input_file = argv[3];
    char *output_file = argv[4];

    // Get the lengths of the input and output names.
    int length_input = strlen(input_file);
    int length_output = strlen(output_file);

    // Obtain the last four characters of the filenames.
    const char *input_extension = &input_file[length_input - 6];
    const char *output_extension = &output_file[length_output - 4];

    if(strcmp(".scene", input_extension) != 0) {
        raytrace_fail("Bad input argument.");
    }

    if(strcmp(".ppm", output_extension) != 0) {
        raytrace_fail("Bad output argument");
    }

    // Open the input file for reading.
    FILE *infile = fopen(input_file, "r"); 

    // Initialize variables to be retrieved from the camera header.
    float camera_width;
    float camera_height;

    // infile is returned to maintain the pointer's position after the camera header.
    infile = get_camera(infile, &camera_width, &camera_height);

    // Check for proper values to confirm proper parsing.
    // printf("Camera Width: %f, Camera Height: %f", camera_width, camera_height);

    // Initialize variables to keep track of the number of each object and light.
    int num_objects;
    int num_lights;
    int num_textures;

    // Starter of the object list if we decide to use a dynamic array
    // object *object_list = malloc(sizeof(object));

    object object_list[MAX_SIZE];
    light light_list[MAX_SIZE];
    texture texture_list[MAX_SIZE];

    get_objects(infile, object_list, light_list, texture_list, &num_objects, 
                &num_lights, &num_textures);

    printf("width: %d, height: %d first_val: %d", 
            texture_list[2].width, texture_list[2].height, texture_list[2].pixmap[0]);

    fclose(infile);


    // Iterates through the object list and prints the object info for error checking.
    // For testing parses. Uncomment to easily view scene data.

    // for(int index = 0; index < num_objects; index++) {

    //     printf("\nObject %d:", index+1);
    //     if(object_list[index].type == Sphere) {
    //         printf("\nType: Sphere,\nRadius: %f,", object_list[index].sphere.radius);
    //     } else {
    //         printf("\nType: Plane,\nNormal: [%f, %f, %f,],", 
    //             object_list[index].plane.normal[0], object_list[index].plane.normal[1],
    //             object_list[index].plane.normal[2]);
    //     }

    //     printf("\nCenter: [%f, %f, %f], ", object_list[index].center[0], 
    //         object_list[index].center[1], object_list[index].center[2]);


    //     printf("\nDiffuse Color: [%d, %d, %d],", object_list[index].diffuse_color[0], 
    //         object_list[index].diffuse_color[1], object_list[index].diffuse_color[2]);

    //     printf("\nSpecular Color: [%d, %d, %d]", object_list[index].specular_color[0], 
    //         object_list[index].specular_color[1], object_list[index].specular_color[2]);

    //     printf("\nReflectivity: %f", object_list[index].reflectivity);

    //     printf("\n\n");

    // }


    // Iterates through the light list and prints the object info for error checking.
    // For testing parses or easily viewing the contents of the scene file.
    
    // for(int index = 0; index < num_lights; index++) {

    //     printf("\nLight %d:", index+1);
    //     if(light_list[index].theta == 0) {
    //         printf("\nType: Point Light,");
    //     } else {
    //         printf("\nType: Spot Light,\nAngular a0: %f,\nDirection: [%f, %f, %f]", 
    //             light_list[index].angular_a0, 
    //             light_list[index].direction[0], light_list[index].direction[1],
    //             light_list[index].direction[2]);
    //         printf("\nCosine of Theta: %f", light_list[index].cosine);
    //     }

    //     printf("\nCenter: [%f, %f, %f],", light_list[index].center[0], 
    //         light_list[index].center[1], light_list[index].center[2]);

    //     printf("\nColor: [%f, %f, %f]", light_list[index].color[0], 
    //         light_list[index].color[1], light_list[index].color[2]);

    //     printf("\nRadial: [%f, %f, %f],", light_list[index].radial[0], 
    //         light_list[index].radial[1], light_list[index].radial[2]);

    //     printf("\nTheta: %f", light_list[index].theta);

    //     printf("\n\n");

    // }

    int arr_length = width * height;
    int max_val = 255;

    // Allocate the memory for the pixmap.
    uint8_t *pixmap = malloc(sizeof(uint8_t) * arr_length * 3);

    if(pixmap == NULL) {
    printf("Error: Memory allocation for pixmap has failed!");
    exit(1);
    }

    printf("\nRaytracing scene with %d objects...\n", num_objects);

    // The image is generated using raytraceing and stored in the pixmap.
    raytrace(pixmap, camera_width, camera_height, object_list, num_objects, light_list, num_lights,
        width, height, texture_list);

    // Close the file since we're done reading from it.
    
    FILE *outfile = fopen(output_file, "w");

    write_p3(outfile, pixmap, width, height, max_val);
    printf("\n");
    printf("File written as P3 format");
    printf("\n");

    // Close the file since we're done writing to it.
    fclose(outfile);

    // Free the malloc now that we're done using it.
    free(pixmap);

    for(int i = 0; i < num_textures; i++) {

        free(texture_list[i].pixmap);

    }
    
    return 0;
}