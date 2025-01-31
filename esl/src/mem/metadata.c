#include "metadata.h"

bool metadataIsEqual(const Metadata* m1, const Metadata* m2)
{
    return m1->type   == m2->type  &&
           m1->state  == m2->state &&
           m1->length == m2->length;
}

bool metadataIsCommon(const Metadata* m1, const Metadata* m2)
{
    return m1->type   == m2->type  &&
           m1->state  == m2->state;
}
