/**
 * @author H. Carter Edwards  <hcedwar@sandia.gov>
 * @date   June 2008
 */

//
//----------------------------------------------------------------------

#include <iostream>

#include <Shards_BasicTopologies.hpp>

#include <stk_util/parallel/Parallel.hpp>

#include <stk_mesh/base/Entity.hpp>
#include <stk_mesh/base/GetEntities.hpp>

#include <stk_mesh/base/FieldData.hpp>

#include <stk_mesh/fem/FieldDeclarations.hpp>
#include <stk_mesh/fem/TopologyHelpers.hpp>
#include <stk_mesh/fem/EntityTypes.hpp>

enum { SpatialDim   = 3 };

#include <use_cases/centroid_algorithm.hpp>

#include <use_cases/UseCase_4.hpp>

//----------------------------------------------------------------------

namespace stk{
namespace mesh {
namespace use_cases {

UseCase_4_MetaData::UseCase_4_MetaData(const std::vector<std::string> & entity_type_names)
  : m_metaData(entity_type_names),
    m_block_hex20(m_metaData.declare_part(       "block_1", Element )),
    m_block_wedge15(m_metaData.declare_part(     "block_2", Element )),
    m_part_vertex_nodes(m_metaData.declare_part(       "vertex_nodes", Node )),
    m_side_part(m_metaData.declare_part(   "sideset_1", Face )),
    m_coordinates_field(m_metaData.declare_field< VectorFieldType >( "coordinates" )),
    m_velocity_field(m_metaData.declare_field< VectorFieldType >( "velocity" )),
    m_centroid_field(m_metaData.declare_field< VectorFieldType >( "centroid" )),
    m_temperature_field(m_metaData.declare_field< ScalarFieldType >( "temperature" )),
    m_pressure_field(m_metaData.declare_field< ScalarFieldType >( "pressure" )),
    m_boundary_field(m_metaData.declare_field< VectorFieldType >( "boundary" )),
    m_element_node_coordinates_field(m_metaData.declare_field< ElementNodePointerFieldType >( "elem_node_coord" ))
{
  // Attache a cell topology to these parts:
  set_cell_topology< shards::Hexahedron<20> >( m_block_hex20 );
  set_cell_topology< shards::Wedge<15>      >( m_block_wedge15 );

  //--------------------------------
  // The vertex nodes of the hex and wedge elements are members
  // of the vertex part; however, the mid-edge nodes are not.
  //
  // Use an element-node stencil to define this relationship.

  // Declare that the Hexahedron<>  nodes of an element in the
  // hex20 element block are members of the linear part.

  m_metaData.declare_part_relation(
    m_block_hex20 ,
    & element_node_stencil< shards::Hexahedron<8>  > ,
    m_part_vertex_nodes );

  // Declare that the Wedge<>  nodes of an element in the
  // wedge15 element block are members of the vertex part.

  m_metaData.declare_part_relation(
    m_block_wedge15 ,
    & element_node_stencil< shards::Wedge<6>  > ,
    m_part_vertex_nodes );

  // Field restrictions:
  Part & universal = m_metaData.universal_part();

  put_field( m_coordinates_field , Node , universal );
  put_field( m_velocity_field , Node , universal );
  put_field( m_centroid_field , Element , universal );
  put_field( m_temperature_field, Node, universal );

  // The pressure field only exists on the vertex nodes:
  put_field( m_pressure_field, Node, m_part_vertex_nodes );

  // The boundary field only exists on nodes in the sideset part
  put_field( m_boundary_field, Node, m_side_part );
  
  m_metaData.declare_field_relation(
    m_element_node_coordinates_field ,
    & element_node_stencil<void> ,
    m_coordinates_field 
    );

  put_field( m_element_node_coordinates_field, Element, m_block_hex20, shards::Hexahedron<20> ::node_count );
  put_field( m_element_node_coordinates_field, Element, m_block_wedge15, shards::Wedge<15> ::node_count );
  
  m_metaData.commit();

}

UseCase_4_Mesh::~UseCase_4_Mesh() 
{
}

UseCase_4_Mesh::UseCase_4_Mesh( stk::ParallelMachine comm )
  : UseCase_4_MetaData(fem_entity_type_names()),
    m_bulkData(m_metaData,comm,field_data_chunk_size)
{
}


enum { node_count   = 66 };
enum { number_hex   = 2 };
enum { number_wedge = 3 };

namespace {

static const double node_coord_data[ node_count ][ SpatialDim ] = {
  {  0 ,  0 ,  0 } , {  0 ,  0 , -1 } , {  0 ,  0 , -2 } ,
  {  1 ,  0 ,  0 } , {  1 ,  0 , -1 } , {  1 ,  0 , -2 } ,
  {  2 ,  0 ,  0 } , {  2 ,  0 , -1 } , {  2 ,  0 , -2 } ,
  {  3 ,  0 ,  0 } , {  3 ,  0 , -1 } , {  3 ,  0 , -2 } ,
  {  4 ,  0 ,  0 } , {  4 ,  0 , -1 } , {  4 ,  0 , -2 } ,

  {  0 ,  1 ,  0 } , {  0 ,  1 , -1 } , {  0 ,  1 , -2 } ,
  {  1 ,  1 ,  0 } , {  1 ,  1 , -1 } , {  1 ,  1 , -2 } ,
  {  2 ,  1 ,  0 } , {  2 ,  1 , -1 } , {  2 ,  1 , -2 } ,
  {  3 ,  1 ,  0 } , {  3 ,  1 , -1 } , {  3 ,  1 , -2 } ,
  {  4 ,  1 ,  0 } , {  4 ,  1 , -1 } , {  4 ,  1 , -2 } ,

  {  0 ,  2 ,  0 } , {  0 ,  2 , -1 } , {  0 ,  2 , -2 } ,
  {  1 ,  2 ,  0 } , {  1 ,  2 , -1 } , {  1 ,  2 , -2 } ,
  {  2 ,  2 ,  0 } , {  2 ,  2 , -1 } , {  2 ,  2 , -2 } ,
  {  3 ,  2 ,  0 } , {  3 ,  2 , -1 } , {  3 ,  2 , -2 } ,
  {  4 ,  2 ,  0 } , {  4 ,  2 , -1 } , {  4 ,  2 , -2 } ,

  {  0.5 , 3 , 0 } , { 0.5 , 3 , -1 } , { 0.5 , 3 , -2 } ,
  {  1.5 , 3 , 0 } , { 1.5 , 3 , -1 } , { 1.5 , 3 , -2 } ,
  {  2.5 , 3 , 0 } , { 2.5 , 3 , -1 } , { 2.5 , 3 , -2 } ,
  {  3.5 , 3 , 0 } , { 3.5 , 3 , -1 } , { 3.5 , 3 , -2 } ,

  {  1 , 4 , 0 } , { 1 , 4 , -1 } , { 1 , 4 , -2 } ,
  {  2 , 4 , 0 } , { 2 , 4 , -1 } , { 2 , 4 , -2 } ,
  {  3 , 4 , 0 } , { 3 , 4 , -1 } , { 3 , 4 , -2 }
};

static const EntityId wedge_node_ids[3][ shards::Wedge<18>::node_count ] = {
  { 33 , 39 , 60 , 31 , 37 , 58 ,
    36 , 51 , 48 , 32 , 38 , 59 , 34 , 49 , 46 ,
    35 , 50 , 47 },
  { 39 , 45 , 66 , 37 , 43 , 64 ,
    42 , 57 , 54 , 38 , 44 , 65 , 40 , 55 , 52 ,
    41 , 56 , 53 },
  { 66 , 60 , 39 , 64 , 58 , 37 ,
    63 , 51 , 54 , 65 , 59 , 38 , 61 , 49 , 52 ,
    62 , 50 , 53 }
};

static const EntityId hex_node_ids[2][ shards::Hexahedron<27>::node_count ] = {
  {  1 ,  7 ,  9 ,  3 , 31 , 37 , 39 , 33 ,
     4 ,  8 ,  6 ,  2 , 16 , 22 , 24 , 18 , 34 , 38 , 36 , 32 ,
    20 ,  5 , 35 , 17 , 23 , 19 , 21 } ,
  {  7 , 13 , 15 ,  9 , 37 , 43 , 45 , 39 ,
    10 , 14 , 12 ,  8 , 22 , 28 , 30 , 24 , 40 , 44 , 42 , 38 ,
    26 , 11 , 41 , 23 , 29 , 25 , 27 }
};

}


void populate( UseCase_4_Mesh & mesh )
{
  BulkData & bulkData = mesh.modifiableBulkData();
  const VectorFieldType & node_coord = mesh.const_coordinates_field();
  Part & block_hex20 = mesh.block_hex20();
  Part & block_wedge15 = mesh.block_wedge15();
  Part & side_part = mesh.side_part();

  EntityId elem_id = 1;
  EntityId face_id = 1;

  PartVector side_add; 
  insert( side_add , side_part );

  for ( unsigned i = 0 ; i < number_hex ; ++i , ++elem_id , ++face_id ) {
    Entity & elem =
      declare_element( bulkData, block_hex20, elem_id, hex_node_ids[i] );

    Entity & face = declare_element_side( bulkData, face_id, elem, 0 );

    bulkData.change_entity_parts( face , side_add );
  }

  for ( unsigned i = 0 ; i < number_wedge ; ++i , ++elem_id , ++face_id ) {
    Entity & elem =
      declare_element( bulkData, block_wedge15, elem_id, wedge_node_ids[i] );

    Entity & face = declare_element_side( bulkData, face_id , elem , 4 );

    bulkData.change_entity_parts( face , side_add );
  }

  for ( unsigned i = 0 ; i < node_count ; ++i ) {
    Entity * const node = bulkData.get_entity( Node, i + 1 );

    if ( node != NULL ) {
      double * const coord = field_data( node_coord , *node );

      coord[0] = node_coord_data[i][0] ;
      coord[1] = node_coord_data[i][1] ;
      coord[2] = node_coord_data[i][2] ;
    }
  }
  
  // No parallel stuff for now
}

void runAlgorithms( UseCase_4_Mesh & mesh ) {
  BulkData & bulkData = mesh.modifiableBulkData();
  VectorFieldType & centroid_field = mesh.centroid_field();
  ElementNodePointerFieldType & elem_node_coord = mesh.element_node_coordinates_field();
  Part & block_hex20 = mesh.block_hex20();
  Part & block_wedge15 = mesh.block_wedge15();

  // Run the centroid algorithms 
  centroid_algorithm< shards::Hexahedron<20> >( bulkData ,
                                        centroid_field ,
                                        elem_node_coord ,
                                        block_hex20 );

  centroid_algorithm< shards::Wedge<15> >( bulkData ,
                                   centroid_field ,
                                   elem_node_coord ,
                                   block_wedge15 );

}

// Note:  this is a duplicate of verify_elem_node_coord_3
bool verify_elem_node_coord_4( 
  Entity & elem ,
  const ElementNodePointerFieldType & elem_node_coord ,
  const VectorFieldType & node_coord ,
  const unsigned node_count )
{
  bool result = true;
  PairIterRelation rel = elem.relations( Node );

  if ( (unsigned) rel.size() != node_count ) {
    std::cerr << "Error!" << std::endl;
    result = false;
  }

  EntityArray< ElementNodePointerFieldType >
    elem_node_array( elem_node_coord , elem );

  {
    const unsigned n1 = elem_node_array.dimension<0>();
    if ( n1 != node_count ) {
      std::cerr << "Error!" << std::endl;
      result = false;
    }
    if ( (unsigned) elem_node_array.size() != node_count ) {
      std::cerr << "Error!" << std::endl;
      result = false;
    }
  }

  double * const * const elem_data = elem_node_array.contiguous_data();

  for ( unsigned j = 0 ; j < node_count ; ++j ) {
    Entity & node = * rel[j].entity();

    EntityArray< VectorFieldType > node_coord_array( node_coord , node );

    {
      const unsigned n1 = node_coord_array.dimension<0>();
      if ( n1 != SpatialDim ) {
        std::cerr << "Error!" << std::endl;
        result = false;
      }
      if ( node_coord_array.size() != SpatialDim ) {
        std::cerr << "Error!" << std::endl;
        result = false;
      }
    }

    double * const node_data = node_coord_array.contiguous_data();
    if ( elem_data[j] != node_data ) {
      std::cerr << "Error!" << std::endl;
      result = false;
    }
  }
  return result;
}


// Note:  this is copied from verify_elem_node_coord_by_part_3
bool verify_elem_node_coord_by_part_4(
    Part & part,
    const std::vector<Bucket *> & bucket_vector,
    const ElementNodePointerFieldType & elem_node_coord,
    const VectorFieldType & node_coord,
    const unsigned node_count )
{
  Selector selector(part);
  std::vector<Entity *> entities;
  get_selected_entities( selector, bucket_vector, entities);
  std::vector<Entity *>::iterator entity_it = entities.begin();
  bool result = true;
  for ( ; entity_it != entities.end() ; ++entity_it ) {
    result = result &&
      verify_elem_node_coord_4( 
          **entity_it , elem_node_coord , node_coord , node_count 
          );
  }
  return result;
}


template< class ElemTraits >
bool verify_elem_side_node( const stk::mesh::EntityId * const elem_nodes ,
                            const unsigned local_side ,
                            const mesh::Entity & side )
{
  bool result = true;
  const CellTopologyData * const elem_top = shards::getCellTopologyData< ElemTraits >();

  const CellTopologyData * const side_top = elem_top->side[ local_side ].topology ;
  const unsigned         * const side_node_map = elem_top->side[ local_side ].node ;

  const mesh::PairIterRelation rel = side.relations( mesh::Node );

  for ( unsigned i = 0 ; i < side_top->node_count ; ++i ) {

    if ( elem_nodes[ side_node_map[i] ] != rel[i].entity()->identifier() ) {
      std::cerr << "Error!" << std::endl;
      result = false;
    }
  }
  return result;
}



bool verify_boundary_field_data( const BulkData & mesh ,
                                 Part & side_part ,
                                 const VectorFieldType & boundary_field )
{
  bool result = true;

  unsigned num_side_nodes = 0 ;

  const std::vector<Bucket*> & buckets = mesh.buckets( Node );

  for ( std::vector<Bucket*>::const_iterator
      k = buckets.begin() ; k != buckets.end() ; ++k ) {

    Bucket & bucket = **k ;

    void * data = field_data( boundary_field , bucket.begin() );

    if ( has_superset( bucket, side_part ) ) {

      if (data == NULL) {
        std::cerr << "Error!  data == NULL" << std::endl;
        result = false;
      }
      num_side_nodes += bucket.size();
    }
    else {
      if ( data != NULL ) {
        std::cerr << "Error!  data != NULL" << std::endl;
        result = false;
      }
    }
  }
  if ( num_side_nodes != 20u ) {
    std::cerr << "Error!  num_side_nodes == " << num_side_nodes << " !=  20!" << std::endl;
    result = false;
  }
  return result;
}


template< class Traits_Full ,
          class Traits_Linear ,
          class PressureField ,
          class VelocityField >
bool verify_pressure_velocity_stencil(
  const BulkData & M ,
  const Part     & element_part ,
  const Part     & linear_node_part ,
  const PressureField  & pressure ,
  const VelocityField  & velocity )
{
  typedef Traits_Full   element_traits ;
  typedef Traits_Linear element_linear_traits ;
  typedef typename FieldTraits< PressureField >::data_type scalar_type ;
  typedef typename FieldTraits< VelocityField >::data_type scalar_type_2 ;

  bool result = true;

  StaticAssert< SameType< scalar_type , scalar_type_2 >::value >::ok();

  if ( (int) element_traits::dimension != element_linear_traits::dimension ) {
    std::cerr << "Error!  element_traits::dimension != element_linear_traits::dimension!" << std::endl;
    result = false;
  }
  if ( (int) element_traits::vertex_count != element_linear_traits::vertex_count ) {
    std::cerr << "Error!  element_traits::vertex_nodes != element_linear_traits::vertex_count!" << std::endl;
    result = false;
  }
  if ( (int) element_traits::edge_count != element_linear_traits::edge_count ) {
    std::cerr << "Error!  element_traits::edge_count != element_linear_traits::edge_count!" << std::endl;
    result = false;
  }
  if ( (int) element_traits::face_count != element_linear_traits::face_count ) {
    std::cerr << "Error!  element_traits::face_count != element_linear_traits::face_count!" << std::endl;
    result = false;
  }

  const std::vector<Bucket*> & buckets = M.buckets( Element );

  for ( std::vector<Bucket*>::const_iterator
        k = buckets.begin() ; k != buckets.end() ; ++k ) {
    Bucket & bucket = **k ;

    if ( has_superset( bucket, element_part ) ) {

      for ( Bucket::iterator
            i = bucket.begin() ; i != bucket.end() ; ++i ) {
        Entity & elem = *i ;

        PairIterRelation rel = elem.relations( Node );

        if ( (unsigned) rel.size() != (unsigned) element_traits::node_count ) {
          std::cerr << "Error!" << std::endl;
          result = false;
        }

        for ( unsigned j = 0 ; j < element_traits::node_count ; ++j ) {
          Entity & node = * rel[j].entity();
          const Bucket & node_bucket = node.bucket();
          PartVector node_parts ;

          node_bucket.supersets( node_parts );

          scalar_type * const p = field_data( pressure , node );
          scalar_type * const v = field_data( velocity , node );

          if ( j < element_linear_traits::node_count ) {

            if ( !contain( node_parts , linear_node_part ) ) {
              std::cerr << "Error!" << std::endl;
              result = false;
            }
            if ( p == NULL ) {
              std::cerr << "Error!" << std::endl;
              result = false;
            }
          }
          else {
            if ( contain( node_parts , linear_node_part ) ) {
              std::cerr << "Error!" << std::endl;
              result = false;
            }
            if ( p != NULL ) {
              std::cerr << "Error!" << std::endl;
              result = false;
            }
          }

          if ( v == NULL ) {
            std::cerr << "Error!" << std::endl;
            result = false;
          }
        }
      }
    }
  }
  return result;
}

bool verifyMesh( const UseCase_4_Mesh & mesh )
{
  bool result = true;
  const BulkData& bulk_data = mesh.bulkData();

  std::vector<Bucket *> element_buckets = bulk_data.buckets( Element );

  // Verify the element node coordinates and side nodes:
  // block_hex20:
  Part & block_hex20 = mesh.block_hex20();
  const ElementNodePointerFieldType & elem_node_coord = mesh.const_element_node_coordinates_field();
  const VectorFieldType & node_coord = mesh.const_coordinates_field();
  result = result && 
    verify_elem_node_coord_by_part_4(
        block_hex20,
        element_buckets,
        elem_node_coord,
        node_coord,
        20
        );
  // Verify element side node: 
  const std::vector<Bucket *> face_buckets = bulk_data.buckets( Face );
  Part & side_part = mesh.side_part();
  {
    Selector selector = block_hex20 & side_part;
    std::vector<Entity *> entities;
    get_selected_entities( selector, face_buckets, entities);
    for (unsigned i=0 ; i < entities.size() ; ++i) {
      result = result &&
        verify_elem_side_node< shards::Hexahedron<20> >( 
          hex_node_ids[i], 0, *entities[i]
          );
    }
  }

  // block_wedge15:
  Part & block_wedge15 = mesh.block_wedge15();
  result = result && 
    verify_elem_node_coord_by_part_4(
        block_wedge15,
        element_buckets,
        elem_node_coord,
        node_coord,
        15
        );
  // Verify element side node: 
  {
    Selector selector = block_wedge15 & side_part;
    std::vector<Entity *> entities;
    get_selected_entities( selector, face_buckets, entities);
    for (unsigned i=0 ; i < entities.size() ; ++i) {
      result = result &&
        verify_elem_side_node< shards::Wedge<15> >( 
          wedge_node_ids[i], 4, *entities[i]
          );
    }
  }

  // Verify centroid dimensions
  const VectorFieldType & centroid_field = mesh.const_centroid_field();
  result = result && 
    centroid_algorithm_unit_test_dimensions< shards::Hexahedron<20> >(
        bulk_data , centroid_field , elem_node_coord , block_hex20 );

  result = result && 
    centroid_algorithm_unit_test_dimensions< shards::Wedge<15> >(
        bulk_data , centroid_field , elem_node_coord , block_wedge15 );

  // Verify boundary field data
  const VectorFieldType & boundary_field = mesh.const_boundary_field();
  result = result && 
    verify_boundary_field_data( bulk_data ,
                              side_part ,
                              boundary_field );

  // Verify pressure velocity stencil for block_hex20
  Part & part_vertex_nodes = mesh.part_vertex_nodes();
  const ScalarFieldType & pressure_field = mesh.const_pressure_field();
  const VectorFieldType & velocity_field = mesh.const_velocity_field();
  result = result && 
    verify_pressure_velocity_stencil
    < shards::Hexahedron<20> , shards::Hexahedron<8>  >
    ( bulk_data , block_hex20 , part_vertex_nodes ,
      pressure_field , velocity_field );

  // Verify pressure velocity stencil for block_wedge15
  result = result && 
    verify_pressure_velocity_stencil
    < shards::Wedge<15> , shards::Wedge<6>  >
    ( bulk_data , block_wedge15 , part_vertex_nodes ,
      pressure_field , velocity_field );

  

  return result;
}

} //namespace use_cases 
} //namespace mesh 
} //namespace stk
