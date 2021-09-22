#include <assert.h>
#include "common.h"
#include "point.h"
#include "math.h"

void
point_translate(struct point *p, double x, double y)
{
	p->x = p->x + x;
	p->y = p->y + y;
}

double
point_distance(const struct point *p1, const struct point *p2)
{
	double dist_x = p2->x - p1->x;	
	double dist_y = p2->y - p1->y;
	double dist = sqrt(dist_x * dist_x + dist_y * dist_y);
	return dist;
}

int
point_compare(const struct point *p1, const struct point *p2)
{
	double dist_1 = sqrt(p1->x * p1->x + p1->y * p1->y);
	double dist_2 = sqrt(p2->x * p2->x + p2->y * p2->y);
	int val;
	if(dist_1 < dist_2) {
		val = -1;
	}
	else if(dist_1 == dist_2) {
		val = 0;
	}
	else {
		val = 1;
	}
	return val;
}
