/****************************************************************************
 *
 *
 *  Copyright 2010-2013, VizExpertsIndia Pvt. Ltd. (unpublished)
 *
 *  All rights reserved. This notice is intended as a precaution against
 *  inadvertent publication and does not imply publication or any waiver
 *  of confidentiality. The year included in the foregoing notice is the
 *  year of creation of the work. No part of this work may be used,
 *  reproduced, or transmitted in any form or by any means without the prior
 *  written permission of Vizexperts India Pvt Ltd.
 *
 *
 ***************************************************************************
 */

/// \file ReaderWritterOpenCASCADE.cpp
/// \brief implementation file for osgdb plugin for IGES format
///        contains implementation of ReaderWritterOpenCASCADE class
/// \author Abhishek Bansal

#include "ReaderWriterOpenCASCADE.h"

#include <iostream>

// OpenCascade Headers
#include <TopTools_HSequenceOfShape.hxx>
#include <TopExp_Explorer.hxx>

#include <Standard_ErrorHandler.hxx>
#include <Standard_CString.hxx>
#include <Standard_Macro.hxx>

#include <TopoDS.hxx>
#include <TopoDS_Face.hxx>

#include <BRepMesh_IncrementalMesh.hxx>
#include <BRep_Tool.hxx>
#include <BRepTools.hxx>
#include <BRepGProp_Face.hxx>
#include <BRepBuilderAPI_Sewing.hxx>

#include <ShapeFix_Shape.hxx>

#include <gp_Pnt2d.hxx>
#include <TColgp_Array1OfPnt2d.hxx>

#include <Poly_Triangulation.hxx>
#include <Poly_Array1OfTriangle.hxx>

#include <IGESControl_Reader.hxx>
#include <IGESControl_Writer.hxx>
#include <IGESControl_Controller.hxx>

#include <STEPCAFControl_Reader.hxx>

#include <TDocStd_Document.hxx>
#include <XCAFDoc_ColorTool.hxx>
#include <XCAFDoc_ShapeTool.hxx>
#include <XCAFDoc_Location.hxx>
#include <XCAFDoc_DocumentTool.hxx>

#include <XCAFApp_Application.hxx>

#include <IGESCAFControl_Reader.hxx>

#include <TDF_Label.hxx>
#include <TDF_LabelSequence.hxx>
#include <TDF_ChildIterator.hxx>

#include <Quantity_Color.hxx>

// osg headers
#include<osg/PrimitiveSet>
#include <osg/MatrixTransform>

#include <osgUtil/SmoothingVisitor>

//#define _LOG_DEBUG_

REGISTER_OSGPLUGIN(OpenCASCADE, ReaderWritterOpenCASCADE)

ReaderWritterOpenCASCADE::ReaderWritterOpenCASCADE()
{
    OSG_NOTICE<<"ReaderWritterOpenCASCADE::ReaderWritterOpenCASCADE()"<<std::endl;

    supportsExtension("IGES","IGES file format");
    supportsExtension("iges","IGES file format");
    supportsExtension("IGS","IGS file format");
    supportsExtension("igs","IGS file format");
    supportsExtension("stp","STEP file format");
    supportsExtension("STP","STEP file format");
}

osgDB::ReaderWriter::ReadResult ReaderWritterOpenCASCADE::readNode(const std::string& fileName, const osgDB::ReaderWriter::Options* options) const
{
    // some error handling
    std::string ext = osgDB::getLowerCaseFileExtension(fileName);
    if (!acceptsExtension(ext))
        return ReadResult::FILE_NOT_HANDLED;

    std::string file = osgDB::findDataFile(fileName, options);
    if (file.empty())
        return ReadResult::FILE_NOT_FOUND;

    OSG_INFO << "ReaderWritterOpenCASCADE::readNode(" << file.c_str() << ")\n";

    OCCTKReader reader;
    return reader.igesToOSGGeode(fileName);
}

osgDB::ReaderWriter::WriteResult ReaderWritterOpenCASCADE::writeNode(const osg::Node& /*node*/,const std::string& fileName /*fileName*/,const Options*) const
{
    // some error handling
    std::string ext = osgDB::getLowerCaseFileExtension(fileName);
    if (!acceptsExtension(ext))
        return WriteResult::FILE_NOT_HANDLED;

    std::cout << "File Writing not supported yet" << std::endl;
    return WriteResult::FILE_NOT_HANDLED;
}

/// \brief heals a opencascade shape
/// \detail http://www.opencascade.org/org/forum/thread_12716/?forum=3
///         Usually IGES files suffer from precision problems (when transferring from
///         one CAD system to another).It might be the case that faces are not sewed
///         properly,  or do not have the right precision, and so the tessellator does
///         not treat them like "sewed". this needs to be done for sewing
/// \param shape opencascade shape to be healed
void ReaderWritterOpenCASCADE::OCCTKReader::_healShape(TopoDS_Shape& shape)
{
    #ifdef _LOG_DEBUG_
        std::cout << std::endl << "Going to heal shape!!";
    #endif

    ShapeFix_Shape fixer(shape);
    fixer.Perform();
    shape = fixer.Shape();

    BRepBuilderAPI_Sewing sew;
    sew.Add(shape);
    sew.Perform();
    shape = sew.SewedShape();
}

/// \brief takes and OpenCascadeShape and returns OSG geometry(drawable), which further can be added to a geode
/// \detail it iterates shape and breaks it into faces, builds vertex list, color list and creates geometry
///         transformation is applied to each vertex before storing it into vertex list
///         all vertices are assigned same color
/// \param shape shape to be converted in geometry. Not a const because it needs to be modified if healing
///        is enabled
/// \param color color of geometry
/// \param transformation matrix with which vertex position has to be transformed
osg::ref_ptr<osg::Geometry> ReaderWritterOpenCASCADE::OCCTKReader::_createGeometryFromShape(TopoDS_Shape& shape, const osg::Vec3& geomColor, gp_Trsf& transformation)
{
    // vector to save vertices
    osg::ref_ptr<osg::Vec3Array> vertexList = new osg::Vec3Array();


    osg::Array::Binding colorBinding = osg::Array::BIND_OVERALL;

    // vector to save _colorTool
    osg::ref_ptr<osg::Vec3Array> colorList = new osg::Vec3Array();
    if (colorBinding==osg::Array::BIND_OVERALL)
    {
        colorList->push_back(geomColor);
    }

    // create one osg primitive set
    osg::ref_ptr<osg::DrawElementsUInt> triangleStrip = new osg::DrawElementsUInt(osg::PrimitiveSet::TRIANGLES, 0);
    unsigned int noOfTriangles = 0;

    osg::ref_ptr<osg::Geometry> geom = new osg::Geometry;

    // large vertex datasets work best with VBO.
    geom->setUseVertexBufferObjects(true);

    if(!shape.IsNull())
    {
        // clean any previous triangulation
        BRepTools::Clean(shape);

        //_healShape(shape);

        #ifdef _LOG_DEBUG_
            std::cout << std::endl << "Building a Mesh !!" ;
        #endif

        /// call to incremental mesh on this shape
        /// \todo not sure why this 1 is passed. Its called deflection BTW
        ///       need to find a way to calculate it
        double linearDeflection = 1.0;
        BRepMesh_IncrementalMesh(shape, linearDeflection);

        ///iterate faces
        // this variable will help in keeping track of face indices
        unsigned int index = 0;
        for (TopExp_Explorer ex(shape, TopAbs_FACE); ex.More(); ex.Next())
        {
            TopoDS_Face face = TopoDS::Face(ex.Current());
            TopLoc_Location location;

            /// triangulate current face
            Handle (Poly_Triangulation) triangulation = BRep_Tool::Triangulation(face, location);
            if (!triangulation.IsNull())
            {
                int noOfNodes = triangulation->NbNodes();

                // Store vertices. Build vertex array here
                for(int j = 1; j <= triangulation->NbNodes(); j++)
                {
                    // populate vertex list
                    // Ref: http://www.opencascade.org/org/forum/thread_16694/?forum=3
                    gp_Pnt pt = (triangulation->Node(j)).Transformed(transformation * location.Transformation());
                    vertexList->push_back(osg::Vec3(pt.X(), pt.Y(), pt.Z()));

                    // populate color list
                    if (colorBinding==osg::Array::BIND_PER_VERTEX)
                    {
                        colorList->push_back(geomColor);
                    }
                }

                /// now we need to get face indices for triangles
                // get list of triangle first
                const Poly_Array1OfTriangle& triangles = triangulation->Triangles();

                //No of triangles in this triangulation
                noOfTriangles = triangulation->NbTriangles();

                Standard_Integer v1, v2, v3;
                for (unsigned int j = 1; j <= noOfTriangles; j++)
                {
                    /// If face direction is reversed then we add verticews in reverse order
                    /// order of vertices is important for normal calculation later
                    if (face.Orientation() == TopAbs_REVERSED)
                    {
                        triangles(j).Get(v1, v3, v2);
                    }
                    else
                    {
                        triangles(j).Get(v1, v2, v3);
                    }
                    triangleStrip->push_back(index + v1 - 1);
                    triangleStrip->push_back(index + v2 - 1);
                    triangleStrip->push_back(index + v3 - 1);
                }
                index = index + noOfNodes;
            }
        }

        #ifdef _LOG_DEBUG_
            std::cout << "Creating a geometry.." << std::endl;
        #endif

        geom->setVertexArray(vertexList.get());

        geom->setColorArray(colorList.get(), colorBinding);

        #ifdef _LOG_DEBUG_
            std::cout << "Adding Primitive set" << std::endl;
        #endif

        geom->addPrimitiveSet(triangleStrip.get());
    }

    return geom;
}

/// \brief this function is single point of contact for this class.
///        it takes path of IGES file and returns an OpenSceneGraph Geode
///        which directly can be used anywhere. It calculates normals using osgUtil::smoother
osg::ref_ptr<osg::Geode> ReaderWritterOpenCASCADE::OCCTKReader::igesToOSGGeode(const std::string& filePath)
{
    // XDE: Extended Data Exchange
    // OCAF: OpenCascade Application Technology Framework
    /// Getting an XDE document
    Handle(TDocStd_Document) doc;
    XCAFApp_Application::GetApplication()->NewDocument("MDTV-XCAF", doc);

    std::string ext = osgDB::getLowerCaseFileExtension(filePath);
    if (ext=="stp" || ext=="step")
    {
        OSG_NOTICE<<"Using STEPCAFControl_Reader to read file : "<<filePath<<std::endl;
        STEPCAFControl_Reader reader;
        reader.SetColorMode(true);
        reader.SetNameMode(true);
        reader.SetLayerMode(true);

        //IGESControl_Reader Reader;
        reader.ReadFile( (Standard_CString)filePath.c_str() );
        /// transfer data from reader to doc
        if(!reader.Transfer(doc))
        {
            std::cout << "Cannot read any relevant data from the STEP file" << std::endl;
            return NULL;
        }
    }
    else
    {
        OSG_NOTICE<<"Using IGESCAFControl_Reader to read file : "<<filePath<<std::endl;
        IGESCAFControl_Reader reader;
        reader.SetColorMode(true);
        reader.SetNameMode(true);
        reader.SetLayerMode(true);

        //IGESControl_Reader Reader;
        reader.ReadFile( (Standard_CString)filePath.c_str() );
        /// transfer data from reader to doc
        if(!reader.Transfer(doc))
        {
            std::cout << "Cannot read any relevant data from the IGES file" << std::endl;
            return NULL;
        }
    }

    // To get a node considered as an Assembly from an XDE structure, you can use the Label of the node.
    _assembly = XCAFDoc_DocumentTool::ShapeTool(doc->Main());

    // To query, edit, or initialize a Document to handle Colors of XCAF
    _colorTool = XCAFDoc_DocumentTool::ColorTool(doc->Main());

    // free shape sequence
    // get sequence of free shape labels
    TDF_LabelSequence freeShapes;
    _assembly->GetFreeShapes(freeShapes);

    if(freeShapes.Length() == 0)
    {
        std::cout << "No Shapes found" << std::endl;
        return NULL;
    }
    else
    {
        std::cout << std::endl << "No of Free Shapes: " << freeShapes.Length();
    }

    _modelGeode = new osg::Geode();
    /// send all root nodes for recursive traversing
    /// find transformation as it will be needed for location calculation later
    for (int i = 1; i <= freeShapes.Length(); i++)
    {
        Handle(XCAFDoc_Location) attribute;
        gp_Trsf transformation;
        freeShapes.Value(i).FindAttribute(XCAFDoc_Location::GetID(), attribute);
        if(attribute.IsNull() == Standard_False)
        {
                TopLoc_Location location = attribute->Get();
                transformation = location.Transformation();
        }
        _traverse(freeShapes.Value(i), transformation);
    }

    /// calculate normals
    #ifdef _LOG_DEBUG_
        std::cout << "Calculating Normals" << std::endl;
    #endif

    osgUtil::SmoothingVisitor sv;
    sv.setCreaseAngle(osg::DegreesToRadians(20.0));
    _modelGeode->accept(sv);

    return _modelGeode;
}

/// \brief recursively traverse opencascade assembly structure and build a osg geode
///        this function also finds color for leaf node shapes and calculates transformation from parent
///        to leaf
/// \param shapeTree its a OCT(OpenCascade Technology) XDE document label which might contain children or referred shapes
/// \param transformation contains transformation matrix to be applied
/// \note Simple Shape: is a shape which is not a compound. Its can be a free or non free shape
/// \note Support Thread: http://www.opencascade.org/org/forum/thread_25512/?forum=3
void ReaderWritterOpenCASCADE::OCCTKReader::_traverse(const TDF_Label &shapeTree, gp_Trsf& transformation)
{
    TDF_Label referredShape;
    /// find if current shape referes some shape. if it does then traverse that
    /// else it is a simple shape and do visualize that simple shape
    if(_assembly->GetReferredShape(shapeTree, referredShape))
    {
        Handle(XCAFDoc_Location) attribute;
        referredShape.FindAttribute(XCAFDoc_Location::GetID(), attribute);
        if(attribute.IsNull() == Standard_False)
        {
                TopLoc_Location location = attribute->Get();
                transformation *= location.Transformation();
        }

        /// if referred shape has children traverse them first else
        /// traverse the shape itself
        if(referredShape.HasChild())
        {
            TDF_ChildIterator it;
            for(it.Initialize(referredShape); it.More(); it.Next())
            {
                _traverse(it.Value(), transformation);
            }
        }
        else
        {
            #ifdef _LOG_DEBUG_
                std::cout << std::endl << "No children found";
            #endif
            _traverse(referredShape, transformation);
        }
    }
    else
    {
        /// Find out if this simple shape has any color store that color as color of geometry
        Quantity_Color color;
        osg::Vec3 geomColor = osg::Vec3(.7, .7, .7);
        if(_colorTool->GetColor(shapeTree, XCAFDoc_ColorGen, color) ||
            _colorTool->GetColor(shapeTree, XCAFDoc_ColorSurf, color) ||
            _colorTool->GetColor(shapeTree, XCAFDoc_ColorCurv, color) )
        {
            #ifdef _LOG_DEBUG_
                std::cout << std::endl << "Free Shape has a color !! " << color.Red() << " " << color.Green() << " "<< color.Blue();
            #endif
            geomColor = osg::Vec3(color.Red(),color.Green(), color.Blue());
        }

        TopoDS_Shape shape = _assembly->GetShape(shapeTree);

        Handle(XCAFDoc_Location) attribute;
        shapeTree.FindAttribute(XCAFDoc_Location::GetID(), attribute);
        if(attribute.IsNull() == Standard_False)
        {
                TopLoc_Location location = attribute->Get();
                transformation *= location.Transformation();
        }

        osg::ref_ptr<osg::Geometry> geom = _createGeometryFromShape(shape, geomColor, transformation);
        /// add this geometry to model geode
        if(geom.valid())
        {
            _modelGeode->addDrawable(geom);
        }
        else
        {
            std::cout << std::endl << "Invalid Geometry found !!";
        }
    }
}
