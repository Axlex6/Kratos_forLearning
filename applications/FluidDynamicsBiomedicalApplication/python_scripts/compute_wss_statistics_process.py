# Importing the Kratos Library
import KratosMultiphysics

# Import applications
import KratosMultiphysics.FluidDynamicsBiomedicalApplication as FluidDynamicsBiomedicalApplication

# # Import base class file
# from FluidDynamicsBiomedicalApplication import WssStatisticsUtilities as wss_stadistics


def Factory(settings, model):
    if(type(settings) != KratosMultiphysics.Parameters):
        raise Exception("expected input shall be a Parameters object, encapsulating a json string")
    return ComputeWssStatisticsProcess(model, settings["Parameters"])


class ComputeWssStatisticsProcess(KratosMultiphysics.Process):
    """
    Auxiliary base class to output total WSS forces
    A derived class needs to be implemented to be able to use this functionality, as calling the base class alone is not enough.
    """

    def __init__(self, model, settings):
        # Base class constructor call
        KratosMultiphysics.Process.__init__(self)

        # Check default settings
        default_settings = KratosMultiphysics.Parameters("""
        {
            "model_part_name": "please_specify_skin_model_part_name",
            "calculate_wss": true,
            "calculate_osi": true,
            "recompute_normals": false
        }
        """)
        settings.ValidateAndAssignDefaults(default_settings)

        # Save model and settings containers
        self.model = model
        self.settings = settings

    def ExecuteInitialize(self):
        # Compute the normal on the nodes of interest
        # Note that this overwrites the existent nodal normal values
        # Also note that if there is a slip condition that shares part of the WSS model part the corner normals could be altered
        # TODO: Improve the NormalCalculationUtils to accept alternative storage variables (to be discussed)
        if self.settings["recompute_normals"].GetBool():
            skin_model_part = self.model.GetModelPart(self.settings["model_part_name"].GetString())
            KratosMultiphysics.NormalCalculationUtils().CalculateOnSimplex(
                skin_model_part,
                skin_model_part.ProcessInfo[KratosMultiphysics.DOMAIN_SIZE])

    def ExecuteFinalizeSolutionStep(self):
        # model_part = self.model.GetModelPart(self.settings["model_part_name"].GetString())
        skin_model_part = self.model.GetModelPart(self.settings["model_part_name"].GetString())

        if self.settings["recompute_normals"].GetBool():
            print ("Computing NORMAL ---------------------------------------------------------------------->")
            self.ExecuteInitialize()

        if self.settings["calculate_wss"].GetBool():
            print ("Computing WSS ----------------------------------------------------------------------->")
            FluidDynamicsBiomedicalApplication.WssStatisticsUtilities.CalculateWSS(skin_model_part)
            #,skin_model_part)

        if self.settings["calculate_osi"].GetBool():
            print ("Computing TWSS ----------------------------------------------------------------------->")
            FluidDynamicsBiomedicalApplication.WssStatisticsUtilities.CalculateTWSS(skin_model_part)
            print ("Computing OSI ----------------------------------------------------------------------->")
            # FluidDynamicsBiomedicalApplication.WssStatisticsUtilities.CalculateOSI(skin_model_part)


    # def ExecuteFinalize(self):
    #     model_part = self.model.GetModelPart(self.settings["model_part_name"].GetString())
    #     skin_model_part = self.model.GetModelPart(self.settings["skin_model_part"].GetString())
    #     if (self.settings["calculate_osi"].GetBool()):
    #         print ("Computing OSI ----------------------------------------------------------------------->")
    #         FluidDynamicsBiomedicalApplication.WssStatisticsUtilities.CalculateOSI(skin_model_part)
    #         FluidDynamicsBiomedicalApplication.WssStatisticsUtilities.CalculateTWSS(skin_model_part)