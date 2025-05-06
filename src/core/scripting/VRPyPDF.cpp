#include "VRPyPDF.h"

#include "VRPyBaseT.h"

using namespace OSG;

simpleVRPyType( PDF, New_ptr );
simpleVRPyType( PDFPage, 0 );

PyMethodDef VRPyPDFPage::methods[] = {
    {"drawLine", PyWrap(PDFPage, drawLine, "Project scene on 2D plane, dir is plane normal", void, Pnt2d, Pnt2d, Color3f, Color3f ) },
    {"drawText", PyWrap(PDFPage, drawText, "Project scene on 2D plane, dir is plane normal", void ) },
    {"project", PyWrap(PDFPage, project, "Project scene on 2D plane, dir is plane normal", void, VRObjectPtr, PosePtr ) },
    {"slice", PyWrap(PDFPage, slice, "Generate a 2D slice of the scene, dir is plane normal", void, VRObjectPtr, PosePtr ) },
    {"extract2DModels", PyWrap(PDFPage, extract2DModels, "Extract all 2D models", vector<VRTechnicalDrawingPtr> ) },
    {"extract3DModels", PyWrap(PDFPage, extract3DModels, "Extract all 3D models", vector<VRTransformPtr> ) },
    {NULL}  /* Sentinel */
};

PyMethodDef VRPyPDF::methods[] = {
    {"read", PyWrap(PDF, read, "Read from file", void, string ) },
    {"write", PyWrap(PDF, write, "Write to file", void, string ) },
    {"getNPages", PyWrap(PDF, getNPages, "Get number of pages", int ) },
    {"addPage", PyWrap(PDF, addPage, "Add page, new page is returned", VRPDFPagePtr ) },
    {"getPage", PyWrap(PDF, getPage, "Get ith page", VRPDFPagePtr, int ) },
    {"remPage", PyWrap(PDF, remPage, "Remove ith page, -1 for last page", void, int ) },
    {NULL}  /* Sentinel */
};
