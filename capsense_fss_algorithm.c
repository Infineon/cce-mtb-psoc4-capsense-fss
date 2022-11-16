/******************************************************************************
* File Name: capsense_fss_algorithm.c
*
* Description: This is the source code for the flanking sensor suppression
*              (FSS) algorithm
*
* Related Document: See README.md
*
*******************************************************************************
* Copyright 2022, Cypress Semiconductor Corporation (an Infineon company) or
* an affiliate of Cypress Semiconductor Corporation.  All rights reserved.
*
* This software, including source code, documentation and related
* materials ("Software") is owned by Cypress Semiconductor Corporation
* or one of its affiliates ("Cypress") and is protected by and subject to
* worldwide patent protection (United States and foreign),
* United States copyright laws and international treaty provisions.
* Therefore, you may use this Software only as provided in the license
* agreement accompanying the software package from which you
* obtained this Software ("EULA").
* If no EULA applies, Cypress hereby grants you a personal, non-exclusive,
* non-transferable license to copy, modify, and compile the Software
* source code solely for use in connection with Cypress's
* integrated circuit products.  Any reproduction, modification, translation,
* compilation, or representation of this Software except as specified
* above is prohibited without the express written permission of Cypress.
*
* Disclaimer: THIS SOFTWARE IS PROVIDED AS-IS, WITH NO WARRANTY OF ANY KIND,
* EXPRESS OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, NONINFRINGEMENT, IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. Cypress
* reserves the right to make changes to the Software without notice. Cypress
* does not assume any liability arising out of the application or use of the
* Software or any product or circuit described in the Software. Cypress does
* not authorize its products for use in any products where a malfunction or
* failure of the Cypress product may reasonably be expected to result in
* significant property damage, injury or death ("High Risk Product"). By
* including Cypress's product in a High Risk Product, the manufacturer
* of such system or application assumes all risk of such use and in doing
* so agrees to indemnify Cypress against all liability.
*******************************************************************************/

/*******************************************************************************
 * Include header files
 ******************************************************************************/
#include "capsense_fss_algorithm.h"

/*******************************************************************************
* Macros
*******************************************************************************/
#define CURRENTBUTTONSTATUS_LSB_MASK   (0x0000000000000001u)
#define RIGHT_SHIFT_1BIT               (1u)

/* Modify FSS_ENABLE_MASK if the FSS algorithm needs to be applied to only
 * certain buttons. Refer README.md for more instructions.
 */
#define FSS_ENABLE_MASK                ((1 << sensorCount) - 1)

/*******************************************************************************
* Global Variables
*******************************************************************************/
uint64_t currentButtonStatus    = 0;
uint64_t previousButtonStatus   = 0;
uint8_t  sensorCount            = 0;

/*******************************************************************************
* Function Prototypes
*******************************************************************************/
static uint64_t fss_algorithm(uint64_t , uint64_t);

/*******************************************************************************
* Function Name: capsense_fss
********************************************************************************
* Summary:
*  This function applies the FSS algorithm on the button sensors. When
*  multiple sensors with FSS enabled are touched, the sensor touched first is 
*  given the higher priority.
*
*******************************************************************************/
void capsense_fss(void)
{
    /* Clearing currentButtonStatus and sensorCount for the next iteration */
    currentButtonStatus = 0;
    sensorCount = 0;

    /* Extracting current button statuses and counting the total number of sensors */
    for (uint8_t widget = 0; widget < CY_CAPSENSE_WIDGET_COUNT; widget++)
    {
        if (cy_capsense_context.ptrWdConfig[widget].wdType == CY_CAPSENSE_WD_BUTTON_E)
        {
            for (uint8_t sensor_iter = 0; sensor_iter < cy_capsense_context.ptrWdConfig[widget].numSns; sensor_iter++)
            {
                currentButtonStatus |= ( (cy_capsense_context.ptrWdConfig[widget].ptrSnsContext[sensor_iter].status &
                                       CY_CAPSENSE_SNS_TOUCH_STATUS_MASK) << sensorCount);
                sensorCount++;
            }
        }
    }

    /* Applying FSS algorithm */
    currentButtonStatus = fss_algorithm(currentButtonStatus,previousButtonStatus);

    /* Storing the current button statuses in previousButtonStatus for the next iteration */
    previousButtonStatus = currentButtonStatus;

    /* Updating the button statuses obtained after the application of FSS algorithm */
    for (uint8_t widget = 0; widget < CY_CAPSENSE_WIDGET_COUNT; widget++)
    {
        if (cy_capsense_context.ptrWdConfig[widget].wdType == CY_CAPSENSE_WD_BUTTON_E)
        {
            for (uint8_t sensor_iter = 0; sensor_iter < cy_capsense_context.ptrWdConfig[widget].numSns; sensor_iter++)
            {
                cy_capsense_context.ptrWdConfig[widget].ptrSnsContext[sensor_iter].status &= (CURRENTBUTTONSTATUS_LSB_MASK &
                                                                                             currentButtonStatus);
                currentButtonStatus >>= RIGHT_SHIFT_1BIT;
            }
        }
    }
}


/*******************************************************************************
* Function Name: fss_algorithm
********************************************************************************
* Summary:
*  This function implements the FSS algorithm
*
*******************************************************************************/
static uint64_t fss_algorithm(uint64_t currentButtonStatus, uint64_t previousButtonStatus)
{
    uint64_t activeFssButtons;
    uint64_t reportedButtons;

    /* FSS algorithm is applied on all buttons */
    uint64_t fssEnableMask = FSS_ENABLE_MASK;

    /* Assume initially that the previous FSS selection is still valid. */
    reportedButtons = previousButtonStatus & fssEnableMask;

    /* Mask off the current FSS group. */
    activeFssButtons = currentButtonStatus & fssEnableMask;

    /* This is non-zero only when a button which was ON previously is still ON,
     * and is in the FSS group.
     */
    if (0 == (activeFssButtons & previousButtonStatus))
    {
        /* The previous FSS selection is no longer active.  Find a new FSS result. */
        for (reportedButtons = 1;
             (0 == (reportedButtons & activeFssButtons)) && (0 != reportedButtons);
             reportedButtons <<= 1)
        {
            /* Do nothing since the loop termination conditions will result in the
             * correct resulting value of reportedButtons.
             */
        }
    }

    /* Combining the status of FSS enabled buttons with the non-FSS enabled buttons */
    return(reportedButtons | (currentButtonStatus & ~fssEnableMask));
}


/* [] END OF FILE */
