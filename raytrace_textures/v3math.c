#include "v3math.h"


// Forms the vector between the heads of vectors a and b.
// head - tail
void v3_from_points(float *dst, float *a, float *b){

    for(int i = 0; i < 3; i++){

        dst[i] = b[i] - a[i];
    }

}


void v3_add(float *dst,float *a, float *b){

    for(int i = 0; i < 3; i++){

        dst[i] = a[i] + b[i];
    }

}


void v3_subtract(float *dst,float *a, float *b){

    for(int i = 0; i < 3; i++){

        dst[i] = a[i] - b[i];

    }

}


// Dot product formula --> a→b=a1b1+a2b2+a3b3
float v3_dot_product(float *a, float *b){

    float dot_product = 0;
    float get_values = 0;
    

    for(int i = 0; i < 3; i++){

        get_values = a[i] * b[i];

        dot_product += get_values;

    }

    return dot_product;

}


void v3_cross_product(float*dst,float *a, float *b){

   dst[0] = (a[1]*b[2]) - (a[2]*b[1]);
   dst[1] = -(a[0]*b[2]) + (a[2]*b[0]);
   dst[2] = (a[0]*b[1]) - (a[1]*b[0]);
  
}


// Ex <2,3,4> scaled by s=2 --> <4,6,8>
void v3_scale(float*dst, float s){

    for(int i = 0; i < 3; i++){

        dst[i] = dst[i] * s;
    }

}

// Calculate the angle between a and b
float v3_angle(float *a, float *b){

    float mag_a;
    float mag_b;
    float angle;
    float dot;

    dot = v3_dot_product(a, b);

    mag_a = sqrtf((a[0]*a[0]) + (a[1]*a[1]) + (a[2]*a[2]));
    mag_b = sqrtf((b[0]*b[0]) + (b[1]*b[1]) + (b[2]*b[2]));

    angle = (mag_a * mag_b);

    // Check to make sure mag_a is not 0 to prevent dividing by 0.
    assert(angle != 0);

    return acos(dot / angle);

}

// Partially calculate the angle between a and b; skipping inverse cosine.
float v3_angle_quick(float *a, float *b){

    float mag_a;
    float mag_b;
    float angle;
    float dot = 0;

    dot = v3_dot_product(a, b);

    mag_a = sqrtf((a[0]*a[0]) + (a[1]*a[1]) + (a[2]*a[2]));
    mag_b = sqrtf((b[0]*b[0]) + (b[1]*b[1]) + (b[2]*b[2]));

    
   // printf("%f", mag_b);

    angle = (mag_a * mag_b);

    // Check to make sure mag_a is not 0 to prevent dividing by 0.
    assert(angle != 0);

    return dot / angle;

}


void v3_reflect(float *dst, float *v, float *n){

    float temp_vector[] = {0.0, 0.0, 0.0};

    // Put the normalized n into temp vector.
    v3_normalize(temp_vector, n);

    // Calculate the scalar to be applied to the normalized n vector.
    float scalar = 2.0 * v3_dot_product(temp_vector, v);

    // Scale the normalized n vector by the calculated scalar: 2(n dot v).
    v3_scale(temp_vector, scalar);

    // Subtract vector v by temp_vector, which contains 2(n dot v)n
    v3_subtract(dst, v, temp_vector);

}


float v3_length(float *a){

    float length;

    length = sqrtf((a[0]*a[0]) + (a[1]*a[1]) + (a[2]*a[2]));

    return length;

}

void v3_normalize(float *dst, float *a){

    float mag_a;

    mag_a = v3_length(a);

    // Check to make sure mag_a is not 0 to prevent dividing by 0.
    assert(mag_a != 0);

    for(int i = 0; i < 3; i++){

        dst[i] = a[i] / mag_a;
    }

}


bool v3_equals(float *a, float *b, float tolerance){

    for(int i = 0; i < 3; i++){

        // Check if the elements are within the given tolerance.
        if(a[i] - b[i] > tolerance || b[i] - a[i] > tolerance){

            return false;
        }
    }
    return true;
}

bool float_equals(float a, float b, float tolerance){

    if(a - b > tolerance || b - a > tolerance){

        return false;
    }

    return true;
}
